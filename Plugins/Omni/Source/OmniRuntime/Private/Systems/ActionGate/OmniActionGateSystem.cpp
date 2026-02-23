#include "Systems/ActionGate/OmniActionGateSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Library/OmniActionLibrary.h"
#include "Manifest/OmniManifest.h"
#include "Profile/OmniActionProfile.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniActionGateSystem, Log, All);

namespace OmniActionGate
{
	static const FName CategoryName(TEXT("ActionGate"));
	static const FName SourceName(TEXT("ActionGateSystem"));
	static const FName SystemId(TEXT("ActionGate"));
	static const FName QueryIsActionActive(TEXT("IsActionActive"));
	static const FName StatusSystemId(TEXT("Status"));
	static const FName ManifestSettingActionProfileAssetPath(TEXT("ActionProfileAssetPath"));
	static const FName ManifestSettingActionProfileClassPath(TEXT("ActionProfileClassPath"));
	static const TCHAR* DefaultActionProfileAssetPath = TEXT("/Game/Data/Action/DA_Omni_ActionProfile_Default.DA_Omni_ActionProfile_Default");
	static const FName DebugMetricProfileAction(TEXT("Omni.Profile.Action"));

	static FString DecisionToResult(const FOmniActionGateDecision& Decision)
	{
		return FString::Printf(TEXT("%s | %s"), Decision.bAllowed ? TEXT("ALLOW") : TEXT("DENY"), *Decision.Reason);
	}
}

FName UOmniActionGateSystem::GetSystemId_Implementation() const
{
	return TEXT("ActionGate");
}

TArray<FName> UOmniActionGateSystem::GetDependencies_Implementation() const
{
	return { OmniActionGate::StatusSystemId };
}

void UOmniActionGateSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);
	bInitialized = false;
	SetInitializationResult(false);

	Registry = Cast<UOmniSystemRegistrySubsystem>(WorldContextObject);
	if (Registry.IsValid())
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>();
		}
	}
	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Pending"));
	}

	DefaultDefinitions.Reset();
	const bool bDevDefaultsEnabled = Registry.IsValid() && Registry->IsDevDefaultsEnabled();
	FString LoadError;
	if (!TryLoadDefinitionsFromManifest(Manifest, bDevDefaultsEnabled, LoadError))
	{
		if (!bDevDefaultsEnabled)
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Error,
				TEXT("Fail-fast [SystemId=%s]: %s"),
				*GetSystemId().ToString(),
				*LoadError
			);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Failed"));
				DebugSubsystem->LogError(OmniActionGate::CategoryName, LoadError, OmniActionGate::SourceName);
			}
			return;
		}

		if (!BuildDevFallbackDefinitionsIfNeeded())
		{
			const FString FallbackError = LoadError.IsEmpty()
				? TEXT("DEV defaults enabled, but ActionGate fallback failed to produce definitions.")
				: FString::Printf(TEXT("%s | DEV defaults fallback also failed."), *LoadError);
			UE_LOG(LogOmniActionGateSystem, Error, TEXT("Fail-fast [SystemId=%s]: %s"), *GetSystemId().ToString(), *FallbackError);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Failed"));
				DebugSubsystem->LogError(OmniActionGate::CategoryName, FallbackError, OmniActionGate::SourceName);
			}
			return;
		}
	}

	RebuildDefinitionMap();
	if (DefinitionsById.Num() == 0)
	{
		const FString EmptyDefinitionsError = FString::Printf(
			TEXT("Fail-fast [SystemId=%s]: no action definitions are available after profile/fallback resolution."),
			*GetSystemId().ToString()
		);
		UE_LOG(LogOmniActionGateSystem, Error, TEXT("%s"), *EmptyDefinitionsError);
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Failed"));
			DebugSubsystem->LogError(OmniActionGate::CategoryName, EmptyDefinitionsError, OmniActionGate::SourceName);
		}
		return;
	}

	ActiveActions.Reset();
	ActiveLockRefCounts.Reset();
	LastDecision = FOmniActionGateDecision();
	bInitialized = true;
	SetInitializationResult(true);
	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Loaded"));
	}

	PublishTelemetry();

	UE_LOG(
		LogOmniActionGateSystem,
		Log,
		TEXT("ActionGate system initialized. Definitions=%d Manifest=%s"),
		DefinitionsById.Num(),
		*GetNameSafe(Manifest)
	);

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->LogEvent(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("ActionGate inicializado. Definicoes=%d"), DefinitionsById.Num()),
			OmniActionGate::SourceName
		);
	}
}

void UOmniActionGateSystem::ShutdownSystem_Implementation()
{
	bInitialized = false;
	DefinitionsById.Reset();
	ActiveActions.Reset();
	ActiveLockRefCounts.Reset();
	LastDecision = FOmniActionGateDecision();

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.KnownActions"));
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.ActiveActions"));
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.ActiveLocks"));
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.LastDecision"));
		DebugSubsystem->RemoveMetric(OmniActionGate::DebugMetricProfileAction);
		DebugSubsystem->LogEvent(OmniActionGate::CategoryName, TEXT("ActionGate finalizado"), OmniActionGate::SourceName);
	}

	DebugSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniActionGateSystem, Log, TEXT("ActionGate system shutdown."));
}

bool UOmniActionGateSystem::IsTickEnabled_Implementation() const
{
	return false;
}

bool UOmniActionGateSystem::HandleCommand_Implementation(const FOmniCommandMessage& Command)
{
	if (Command.CommandName == OmniMessageSchema::CommandStartAction)
	{
		FOmniStartActionCommandSchema ParsedSchema;
		FString ParseError;
		if (!FOmniStartActionCommandSchema::TryFromMessage(Command, ParsedSchema, ParseError))
		{
			return false;
		}

		FOmniActionGateDecision Decision;
		return TryStartAction(ParsedSchema.ActionId, Decision);
	}

	if (Command.CommandName == OmniMessageSchema::CommandStopAction)
	{
		FOmniStopActionCommandSchema ParsedSchema;
		FString ParseError;
		if (!FOmniStopActionCommandSchema::TryFromMessage(Command, ParsedSchema, ParseError))
		{
			return false;
		}

		return StopAction(ParsedSchema.ActionId, ParsedSchema.Reason);
	}

	return Super::HandleCommand_Implementation(Command);
}

bool UOmniActionGateSystem::HandleQuery_Implementation(FOmniQueryMessage& Query)
{
	if (Query.QueryName == OmniMessageSchema::QueryCanStartAction)
	{
		FOmniCanStartActionQuerySchema ParsedSchema;
		FString ParseError;
		if (!FOmniCanStartActionQuerySchema::TryFromMessage(Query, ParsedSchema, ParseError))
		{
			Query.bHandled = true;
			Query.bSuccess = false;
			Query.Result = ParseError.IsEmpty() ? TEXT("Invalid query payload.") : ParseError;
			return true;
		}

		FOmniActionGateDecision Decision;
		const bool bAllowed = EvaluateStartAction(ParsedSchema.ActionId, Decision, false);
		Query.bHandled = true;
		Query.bSuccess = bAllowed;
		Query.Result = Decision.Reason;
		return true;
	}

	if (Query.QueryName == OmniActionGate::QueryIsActionActive)
	{
		FName ActionId = NAME_None;
		if (!TryParseActionId(Query, ActionId))
		{
			Query.bHandled = true;
			Query.bSuccess = false;
			Query.Result = TEXT("Missing ActionId");
			return true;
		}

		const bool bActive = ActiveActions.Contains(ActionId);
		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = bActive ? TEXT("True") : TEXT("False");
		return true;
	}

	return Super::HandleQuery_Implementation(Query);
}

void UOmniActionGateSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);
	(void)Event;
}

bool UOmniActionGateSystem::TryStartAction(const FName ActionId, FOmniActionGateDecision& OutDecision)
{
	const bool bAllowed = EvaluateStartAction(ActionId, OutDecision, true);
	PublishDecision(OutDecision, true);
	return bAllowed;
}

bool UOmniActionGateSystem::StopAction(const FName ActionId, const FName Reason)
{
	if (!ActiveActions.Contains(ActionId))
	{
		return false;
	}

	const FOmniActionDefinition* Definition = DefinitionsById.Find(ActionId);
	if (!Definition)
	{
		ActiveActions.Remove(ActionId);
		PublishTelemetry();
		return true;
	}

	RemoveActionLocks(*Definition);
	ActiveActions.Remove(ActionId);
	PublishTelemetry();

	if (DebugSubsystem.IsValid())
	{
		const FString StopReason = Reason == NAME_None ? TEXT("Manual") : Reason.ToString();
		DebugSubsystem->LogEvent(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("Acao parada: %s (Reason=%s)"), *ActionId.ToString(), *StopReason),
			OmniActionGate::SourceName
		);
	}

	return true;
}

bool UOmniActionGateSystem::IsActionActive(const FName ActionId) const
{
	return ActiveActions.Contains(ActionId);
}

TArray<FName> UOmniActionGateSystem::GetActiveActions() const
{
	TArray<FName> Result = ActiveActions.Array();
	Result.Sort(FNameLexicalLess());
	return Result;
}

FGameplayTagContainer UOmniActionGateSystem::GetActiveLocks() const
{
	FGameplayTagContainer Result;
	for (const TPair<FGameplayTag, int32>& Pair : ActiveLockRefCounts)
	{
		if (Pair.Key.IsValid() && Pair.Value > 0)
		{
			Result.AddTag(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> UOmniActionGateSystem::GetKnownActionIds() const
{
	TArray<FName> Result;
	DefinitionsById.GenerateKeyArray(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

FOmniActionGateDecision UOmniActionGateSystem::GetLastDecision() const
{
	return LastDecision;
}

bool UOmniActionGateSystem::TryLoadDefinitionsFromManifest(
	const UOmniManifest* Manifest,
	const bool bAllowDevDefaults,
	FString& OutError
)
{
	OutError.Reset();

	if (!Manifest)
	{
		OutError = TEXT("Manifest is null.");
		return false;
	}

	const FName RuntimeSystemId = GetSystemId();
	const FOmniSystemManifestEntry* SystemEntry = Manifest->FindEntryById(RuntimeSystemId);
	if (!SystemEntry)
	{
		SystemEntry = Manifest->Systems.FindByPredicate(
			[this](const FOmniSystemManifestEntry& Entry)
			{
				UClass* LoadedClass = Entry.SystemClass.LoadSynchronous();
				return LoadedClass == GetClass();
			}
		);
	}
	if (!SystemEntry)
	{
		OutError = FString::Printf(
			TEXT("SystemId '%s' not found in manifest '%s'."),
			*RuntimeSystemId.ToString(),
			*GetNameSafe(Manifest)
		);
		return false;
	}

	UOmniActionProfile* LoadedProfile = nullptr;
	bool bLoadedFromAssetPath = false;
	bool bLoadedFromClassPath = false;

	FString ProfileAssetPathValue;
	if (!SystemEntry->TryGetSetting(OmniActionGate::ManifestSettingActionProfileAssetPath, ProfileAssetPathValue)
		|| ProfileAssetPathValue.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("Missing required manifest setting '%s' for SystemId '%s'. Expected path: %s. Recommendation: create a UOmniActionProfile asset at the expected path and reference a UOmniActionLibrary."),
			*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString(),
			*RuntimeSystemId.ToString(),
			OmniActionGate::DefaultActionProfileAssetPath
		);
	}
	else
	{
		const FSoftObjectPath ProfileAssetPath(ProfileAssetPathValue);
		if (ProfileAssetPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Invalid '%s' value for SystemId '%s': '%s'. Recommendation: set a valid UOmniActionProfile asset path (expected: %s)."),
				*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString(),
				*RuntimeSystemId.ToString(),
				*ProfileAssetPathValue,
				OmniActionGate::DefaultActionProfileAssetPath
			);
		}
		else
		{
			UObject* LoadedObject = ProfileAssetPath.TryLoad();
			if (!LoadedObject)
			{
				OutError = FString::Printf(
					TEXT("Action profile asset not found for SystemId '%s': %s. Recommendation: create a UOmniActionProfile asset at this path and reference a UOmniActionLibrary."),
					*RuntimeSystemId.ToString(),
					*ProfileAssetPath.ToString()
				);
			}
			else if (UOmniActionProfile* CastedProfile = Cast<UOmniActionProfile>(LoadedObject))
			{
				LoadedProfile = CastedProfile;
				bLoadedFromAssetPath = true;
			}
			else
			{
				OutError = FString::Printf(
					TEXT("Asset type mismatch for SystemId '%s': %s is '%s' (expected UOmniActionProfile). Recommendation: create a UOmniActionProfile asset at this path."),
					*RuntimeSystemId.ToString(),
					*ProfileAssetPath.ToString(),
					*LoadedObject->GetClass()->GetPathName()
				);
			}
		}
	}

	if (!LoadedProfile && bAllowDevDefaults)
	{
		FString ProfileClassPathValue;
		if (SystemEntry->TryGetSetting(OmniActionGate::ManifestSettingActionProfileClassPath, ProfileClassPathValue))
		{
			const FSoftClassPath ProfileClassPath(ProfileClassPathValue);
			if (!ProfileClassPath.IsNull())
			{
				UClass* LoadedClass = ProfileClassPath.TryLoadClass<UOmniActionProfile>();
				if (!LoadedClass || !LoadedClass->IsChildOf(UOmniActionProfile::StaticClass()))
				{
					if (OutError.IsEmpty())
					{
						OutError = FString::Printf(
							TEXT("Action profile class fallback invalid for SystemId '%s': %s."),
							*RuntimeSystemId.ToString(),
							*ProfileClassPath.ToString()
						);
					}
					else
					{
						OutError += FString::Printf(TEXT(" Class fallback invalid: %s."), *ProfileClassPath.ToString());
					}
				}
				else
				{
					LoadedProfile = NewObject<UOmniActionProfile>(this, LoadedClass, NAME_None, RF_Transient);
					bLoadedFromClassPath = true;
				}
			}
		}
	}

	if (!LoadedProfile)
	{
		if (OutError.IsEmpty())
		{
			OutError = FString::Printf(
				TEXT("Failed to resolve action profile for SystemId '%s'. Expected path: %s."),
				*RuntimeSystemId.ToString(),
				OmniActionGate::DefaultActionProfileAssetPath
			);
		}
		return false;
	}

	if (bLoadedFromAssetPath)
	{
		const FSoftObjectPath ActionLibraryPath = LoadedProfile->ActionLibrary.ToSoftObjectPath();
		if (ActionLibraryPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Profile '%s' for SystemId '%s' has null ActionLibrary. Recommendation: set ActionLibrary to a UOmniActionLibrary asset."),
				*GetNameSafe(LoadedProfile),
				*RuntimeSystemId.ToString()
			);
			return false;
		}

		UObject* LoadedLibraryObject = ActionLibraryPath.TryLoad();
		if (!LoadedLibraryObject)
		{
			OutError = FString::Printf(
				TEXT("ActionLibrary not found for SystemId '%s': %s. Recommendation: create a UOmniActionLibrary asset and assign it in profile '%s'."),
				*RuntimeSystemId.ToString(),
				*ActionLibraryPath.ToString(),
				*GetNameSafe(LoadedProfile)
			);
			return false;
		}
		if (!Cast<UOmniActionLibrary>(LoadedLibraryObject))
		{
			OutError = FString::Printf(
				TEXT("ActionLibrary type mismatch for SystemId '%s': %s is '%s' (expected UOmniActionLibrary)."),
				*RuntimeSystemId.ToString(),
				*ActionLibraryPath.ToString(),
				*LoadedLibraryObject->GetClass()->GetPathName()
			);
			return false;
		}
	}

	TArray<FOmniActionDefinition> LoadedDefinitions;
	LoadedProfile->ResolveDefinitions(LoadedDefinitions);
	if (LoadedDefinitions.Num() == 0)
	{
		OutError = FString::Printf(
			TEXT("Action profile '%s' resolved zero definitions for SystemId '%s'. Recommendation: populate ActionLibrary definitions."),
			*GetNameSafe(LoadedProfile),
			*RuntimeSystemId.ToString()
		);
		return false;
	}

	DefaultDefinitions = MoveTemp(LoadedDefinitions);
	if (bLoadedFromClassPath || LoadedProfile->GetClass()->IsChildOf(UOmniDevActionProfile::StaticClass()))
	{
		UE_LOG(
			LogOmniActionGateSystem,
			Warning,
			TEXT("Action definitions loaded from DEV_FALLBACK profile class: %s"),
			*LoadedProfile->GetClass()->GetPathName()
		);
	}
	else
	{
		UE_LOG(
			LogOmniActionGateSystem,
			Log,
			TEXT("Action definitions loaded from profile: %s (Definitions=%d)"),
			*GetNameSafe(LoadedProfile),
			DefaultDefinitions.Num()
		);
	}

	OutError.Reset();
	return true;
}

bool UOmniActionGateSystem::BuildDevFallbackDefinitionsIfNeeded()
{
	if (DefaultDefinitions.Num() > 0)
	{
		return true;
	}

	static bool bWarnedDevDefaultsThisSession = false;
	if (!bWarnedDevDefaultsThisSession)
	{
		UE_LOG(LogOmniActionGateSystem, Warning, TEXT("DEV_DEFAULTS ACTIVE: ActionGate is using fallback definitions."));
		bWarnedDevDefaultsThisSession = true;
	}

	UOmniDevActionProfile* DevProfile = NewObject<UOmniDevActionProfile>(this, NAME_None, RF_Transient);
	if (DevProfile)
	{
		DevProfile->ResolveDefinitions(DefaultDefinitions);
	}

	if (DefaultDefinitions.Num() > 0)
	{
		UE_LOG(LogOmniActionGateSystem, Warning, TEXT("Action definitions loaded from DEV_FALLBACK profile instance."));
		return true;
	}

	UE_LOG(LogOmniActionGateSystem, Warning, TEXT("Action definitions loaded from DEV_FALLBACK inline C++."));

	FOmniActionDefinition SprintAction;
	SprintAction.ActionId = TEXT("Movement.Sprint");
	SprintAction.Policy = EOmniActionPolicy::DenyIfActive;

	const FGameplayTag ExhaustedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Exhausted"), false);
	if (ExhaustedTag.IsValid())
	{
		SprintAction.BlockedBy.AddTag(ExhaustedTag);
	}

	const FGameplayTag SprintLockTag = FGameplayTag::RequestGameplayTag(TEXT("Lock.Movement.Sprint"), false);
	if (SprintLockTag.IsValid())
	{
		SprintAction.AppliesLocks.AddTag(SprintLockTag);
	}

	DefaultDefinitions.Add(MoveTemp(SprintAction));
	return DefaultDefinitions.Num() > 0;
}

void UOmniActionGateSystem::RebuildDefinitionMap()
{
	DefinitionsById.Reset();

	for (const FOmniActionDefinition& Definition : DefaultDefinitions)
	{
		if (Definition.ActionId == NAME_None)
		{
			continue;
		}

		if (DefinitionsById.Contains(Definition.ActionId))
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Warning,
				TEXT("ActionId duplicado em definicoes: %s (ultima definicao vence)."),
				*Definition.ActionId.ToString()
			);
		}

		DefinitionsById.Add(Definition.ActionId, Definition);
	}
}

FGameplayTagContainer UOmniActionGateSystem::BuildCurrentBlockingContext() const
{
	FGameplayTagContainer Context;

	if (Registry.IsValid())
	{
		FOmniGetStateTagsCsvQuerySchema RequestSchema;
		RequestSchema.SourceSystem = OmniActionGate::SystemId;
		FOmniQueryMessage Query = FOmniGetStateTagsCsvQuerySchema::ToMessage(RequestSchema);

		FOmniGetStateTagsCsvQuerySchema ResponseSchema;
		FString ParseError;
		if (Registry->ExecuteQuery(Query)
			&& Query.bSuccess
			&& FOmniGetStateTagsCsvQuerySchema::TryFromMessage(Query, ResponseSchema, ParseError)
			&& !ResponseSchema.TagsCsv.IsEmpty())
		{
			TArray<FString> TagStrings;
			ResponseSchema.TagsCsv.ParseIntoArray(TagStrings, TEXT(","), true);
			for (FString TagString : TagStrings)
			{
				TagString.TrimStartAndEndInline();
				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
				if (Tag.IsValid())
				{
					Context.AddTag(Tag);
				}
			}
		}
	}

	Context.AppendTags(GetActiveLocks());
	return Context;
}

bool UOmniActionGateSystem::EvaluateStartAction(const FName ActionId, FOmniActionGateDecision& OutDecision, const bool bApplyChanges)
{
	FOmniActionGateDecision Decision;
	Decision.ActionId = ActionId;

	if (!bInitialized || ActionId == NAME_None)
	{
		Decision.bAllowed = false;
		Decision.Reason = TEXT("Sistema nao inicializado ou ActionId invalido.");
		OutDecision = Decision;
		return false;
	}

	const FOmniActionDefinition* Definition = DefinitionsById.Find(ActionId);
	if (!Definition || !Definition->bEnabled)
	{
		Decision.bAllowed = false;
		Decision.Reason = TEXT("Acao desconhecida ou desabilitada.");
		OutDecision = Decision;
		return false;
	}

	Decision.Policy = Definition->Policy;

	const bool bAlreadyActive = ActiveActions.Contains(ActionId);
	if (bAlreadyActive)
	{
		if (Definition->Policy == EOmniActionPolicy::DenyIfActive)
		{
			Decision.bAllowed = false;
			Decision.Reason = TEXT("Acao ja ativa (policy deny).");
			OutDecision = Decision;
			return false;
		}

		if (Definition->Policy == EOmniActionPolicy::SucceedIfActive)
		{
			Decision.bAllowed = true;
			Decision.Reason = TEXT("Acao ja ativa (policy succeed).");
			OutDecision = Decision;
			return true;
		}

		if (Definition->Policy == EOmniActionPolicy::RestartIfActive)
		{
			if (bApplyChanges)
			{
				StopAction(ActionId, TEXT("RestartPolicy"));
			}
			Decision.Reason = TEXT("Acao reiniciada (policy restart).");
		}
	}

	const FGameplayTagContainer BlockingContext = BuildCurrentBlockingContext();
	if (Definition->BlockedBy.HasAny(BlockingContext))
	{
		Decision.bAllowed = false;
		Decision.Reason = FString::Printf(TEXT("Bloqueada por tags: %s"), *Definition->BlockedBy.ToStringSimple());
		OutDecision = Decision;
		return false;
	}

	if (bApplyChanges)
	{
		for (const FName ActionToCancel : Definition->Cancels)
		{
			if (ActionToCancel == NAME_None || !ActiveActions.Contains(ActionToCancel))
			{
				continue;
			}

			if (StopAction(ActionToCancel, ActionId))
			{
				Decision.CanceledActions.Add(ActionToCancel);
			}
		}

		ActiveActions.Add(ActionId);
		AddActionLocks(*Definition);
	}

	Decision.bAllowed = true;
	if (Decision.Reason.IsEmpty())
	{
		Decision.Reason = TEXT("Autorizada.");
	}

	OutDecision = Decision;
	return true;
}

bool UOmniActionGateSystem::TryParseActionId(const FOmniQueryMessage& Query, FName& OutActionId)
{
	FString ActionValue;
	if (!Query.TryGetArgument(TEXT("ActionId"), ActionValue) || ActionValue.IsEmpty())
	{
		return false;
	}

	OutActionId = FName(*ActionValue);
	return OutActionId != NAME_None;
}

void UOmniActionGateSystem::AddActionLocks(const FOmniActionDefinition& Definition)
{
	for (const FGameplayTag& LockTag : Definition.AppliesLocks)
	{
		if (!LockTag.IsValid())
		{
			continue;
		}

		int32& Count = ActiveLockRefCounts.FindOrAdd(LockTag);
		Count++;
	}
}

void UOmniActionGateSystem::RemoveActionLocks(const FOmniActionDefinition& Definition)
{
	for (const FGameplayTag& LockTag : Definition.AppliesLocks)
	{
		if (!LockTag.IsValid())
		{
			continue;
		}

		int32* CountPtr = ActiveLockRefCounts.Find(LockTag);
		if (!CountPtr)
		{
			continue;
		}

		*CountPtr = FMath::Max(0, *CountPtr - 1);
		if (*CountPtr == 0)
		{
			ActiveLockRefCounts.Remove(LockTag);
		}
	}
}

void UOmniActionGateSystem::PublishTelemetry()
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("ActionGate.KnownActions"), FString::FromInt(DefinitionsById.Num()));
	DebugSubsystem->SetMetric(TEXT("ActionGate.ActiveActions"), FString::FromInt(ActiveActions.Num()));
	DebugSubsystem->SetMetric(TEXT("ActionGate.ActiveLocks"), FString::FromInt(ActiveLockRefCounts.Num()));
}

void UOmniActionGateSystem::PublishDecision(const FOmniActionGateDecision& Decision, const bool bEmitLogEntry)
{
	LastDecision = Decision;
	PublishTelemetry();

	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("ActionGate.LastDecision"), OmniActionGate::DecisionToResult(Decision));

	if (Registry.IsValid())
	{
		FOmniEventMessage Event;
		Event.SourceSystem = OmniActionGate::SystemId;
		Event.EventName = Decision.bAllowed ? TEXT("ActionAllowed") : TEXT("ActionDenied");
		Event.SetPayloadValue(TEXT("ActionId"), Decision.ActionId.ToString());
		Event.SetPayloadValue(TEXT("Reason"), Decision.Reason);
		Registry->BroadcastEvent(Event);
	}

	if (!bEmitLogEntry)
	{
		return;
	}

	if (Decision.bAllowed)
	{
		DebugSubsystem->LogEvent(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("ALLOW %s (%s)"), *Decision.ActionId.ToString(), *Decision.Reason),
			OmniActionGate::SourceName
		);
	}
	else
	{
		DebugSubsystem->LogWarning(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("DENY %s (%s)"), *Decision.ActionId.ToString(), *Decision.Reason),
			OmniActionGate::SourceName
		);
	}
}
