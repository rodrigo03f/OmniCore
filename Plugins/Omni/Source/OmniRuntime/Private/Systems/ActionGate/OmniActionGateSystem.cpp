#include "Systems/ActionGate/OmniActionGateSystem.h"

#include "Algo/Sort.h"
#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Library/OmniActionLibrary.h"
#include "Manifest/OmniManifest.h"
#include "Profile/OmniActionProfile.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "HAL/IConsoleManager.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniActionGateSystem, Log, All);

namespace OmniActionGate
{
	static const FName CategoryName(TEXT("ActionGate"));
	static const FName SourceName(TEXT("ActionGateSystem"));
	static const FName SystemId(TEXT("ActionGate"));
	static const FName QueryIsActionActive(TEXT("IsActionActive"));
	static const FName StatusSystemId(TEXT("Status"));
	static const FName EventOnActionStarted(TEXT("OnActionStarted"));
	static const FName EventOnActionEnded(TEXT("OnActionEnded"));
	static const FName EventOnActionDenied(TEXT("OnActionDenied"));
	static const FName ManifestSettingActionProfileAssetPath(TEXT("ActionProfileAssetPath"));
	static const FGameplayTag ReasonAllowTag = FGameplayTag::RequestGameplayTag(TEXT("Omni.Gate.Allow.Ok"), false);
	static const FGameplayTag ReasonDenyLockedTag = FGameplayTag::RequestGameplayTag(TEXT("Omni.Gate.Deny.Locked"), false);
	static const TCHAR* DisallowedActionIdPrefix = TEXT("Input.");
	static const FName DebugMetricProfileAction(TEXT("Omni.Profile.Action"));
	static TAutoConsoleVariable<int32> CVarActionGateFailFast(
		TEXT("omni.actiongate.failfast"),
		-1,
		TEXT("ActionGate validation strict mode.\n-1=Auto (strict in non-shipping, non-strict in shipping)\n0=Non-strict (log + deny/sanitize)\n1=Strict fail-fast"),
		ECVF_Default
	);

	static FString DecisionToResult(const FOmniGateDecision& Decision)
	{
		const FString ReasonTagText = Decision.ReasonTag.IsValid() ? Decision.ReasonTag.ToString() : TEXT("<invalid>");
		const FString ReasonText = Decision.ReasonText.IsEmpty() ? TEXT("<none>") : Decision.ReasonText;
		return FString::Printf(TEXT("%s | %s | %s"), Decision.bAllowed ? TEXT("ALLOW") : TEXT("DENY"), *ReasonTagText, *ReasonText);
	}

	static bool IsStrictValidationEnabled()
	{
		const int32 CVarValue = CVarActionGateFailFast.GetValueOnGameThread();
		if (CVarValue == 0)
		{
			return false;
		}
		if (CVarValue == 1)
		{
			return true;
		}

#if UE_BUILD_SHIPPING
		return false;
#else
		return true;
#endif
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
	ResolvedProfileName.Reset();
	ResolvedProfileAssetPath.Reset();
	ResolvedLibraryAssetPath.Reset();
	FString LoadError;
	if (!TryLoadDefinitionsFromManifest(Manifest, LoadError))
	{
		UE_LOG(
			LogOmniActionGateSystem,
			Error,
			TEXT("[Omni][ActionGate][Init] Fail-fast: configuracao invalida | %s"),
			*LoadError
		);
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Failed"));
			DebugSubsystem->LogError(OmniActionGate::CategoryName, LoadError, OmniActionGate::SourceName);
		}
		return;
	}

	RebuildDefinitionMap();
	if (DefinitionsById.Num() == 0)
	{
		const bool bStrictValidation = OmniActionGate::IsStrictValidationEnabled();
		const FString EmptyDefinitionsError = FString::Printf(
			TEXT("No action definitions are available after profile resolution [SystemId=%s Profile=%s]. Configure '%s' in manifest and populate ActionLibrary '%s'."),
			*GetSystemId().ToString(),
			ResolvedProfileName.IsEmpty() ? TEXT("<unknown>") : *ResolvedProfileName,
			*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString(),
			ResolvedLibraryAssetPath.IsEmpty() ? TEXT("<unknown>") : *ResolvedLibraryAssetPath
		);
		if (bStrictValidation)
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Error,
				TEXT("[Omni][ActionGate][Init] Definicoes vazias apos resolver profile | %s"),
				*EmptyDefinitionsError
			);
		}
		else
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Warning,
				TEXT("[Omni][ActionGate][Init] Definicoes vazias apos resolver profile | %s"),
				*EmptyDefinitionsError
			);
		}
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, bStrictValidation ? TEXT("Failed") : TEXT("LoadedWithWarnings"));
			if (bStrictValidation)
			{
				DebugSubsystem->LogError(OmniActionGate::CategoryName, EmptyDefinitionsError, OmniActionGate::SourceName);
			}
			else
			{
				DebugSubsystem->LogWarning(OmniActionGate::CategoryName, EmptyDefinitionsError, OmniActionGate::SourceName);
			}
		}
		if (bStrictValidation)
		{
			return;
		}
	}

	ActiveActions.Reset();
	ActiveLocks.Reset();
	LocksByAction.Reset();
	LastDecision = FOmniGateDecision();
	AllowReasonTag = OmniActionGate::ReasonAllowTag;
	DenyReasonTag = OmniActionGate::ReasonDenyLockedTag;
	if (!AllowReasonTag.IsValid() || !DenyReasonTag.IsValid())
	{
		const FString ReasonTagError = TEXT("Missing required reason tags: Omni.Gate.Allow.Ok / Omni.Gate.Deny.Locked.");
		UE_LOG(LogOmniActionGateSystem, Error, TEXT("[Omni][ActionGate][Init] %s"), *ReasonTagError);
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Failed"));
			DebugSubsystem->LogError(OmniActionGate::CategoryName, ReasonTagError, OmniActionGate::SourceName);
		}
		return;
	}
	bInitialized = true;
	SetInitializationResult(true);
	if (DebugSubsystem.IsValid())
	{
		if (DefinitionsById.Num() > 0)
		{
			DebugSubsystem->SetMetric(OmniActionGate::DebugMetricProfileAction, TEXT("Loaded"));
		}
	}

	PublishTelemetry();

	UE_LOG(
		LogOmniActionGateSystem,
		Log,
		TEXT("[Omni][ActionGate][Init] Inicializado | definitions=%d manifest=%s"),
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
	ActiveLocks.Reset();
	LocksByAction.Reset();
	LastDecision = FOmniGateDecision();
	AllowReasonTag = FGameplayTag();
	DenyReasonTag = FGameplayTag();
	ResolvedProfileName.Reset();
	ResolvedProfileAssetPath.Reset();
	ResolvedLibraryAssetPath.Reset();

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

	UE_LOG(LogOmniActionGateSystem, Log, TEXT("[Omni][ActionGate][Shutdown] Concluido"));
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

		const FOmniGateDecision Decision = StartAction(ParsedSchema.ActionId);
		return Decision.bAllowed;
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

		const FOmniGateDecision Decision = EvaluateStartAction(ParsedSchema.ActionId, false);
		Query.bHandled = true;
		Query.bSuccess = Decision.bAllowed;
		Query.Result = Decision.ReasonTag.IsValid() ? Decision.ReasonTag.ToString() : TEXT("Omni.Gate.Deny.Locked");
		Query.SetOutputValue(TEXT("ReasonTag"), Query.Result);
		Query.SetOutputValue(TEXT("ReasonText"), Decision.ReasonText);
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

FOmniGateDecision UOmniActionGateSystem::Can(const FGameplayTag ActionTag, const FGameplayTagContainer& ContextTags) const
{
	FOmniGateDecision Decision;
	Decision.ReasonTag = DenyReasonTag;

	if (!bInitialized || !ActionTag.IsValid())
	{
		Decision.bAllowed = false;
		Decision.ReasonText = TEXT("ActionTag invalida ou sistema nao inicializado.");
		return Decision;
	}

	const TArray<FOmniGateLock> SortedLocks = BuildSortedLocks();
	for (const FOmniGateLock& Lock : SortedLocks)
	{
		if (Lock.Scope == EOmniActionGateLockScope::Global)
		{
			Decision.bAllowed = false;
			Decision.ReasonText = FString::Printf(
				TEXT("Global lock ativo: %s (source=%s)"),
				*Lock.LockTag.ToString(),
				*Lock.SourceId.ToString()
			);
			return Decision;
		}

		if (Lock.Scope == EOmniActionGateLockScope::Action && Lock.ActionTag == ActionTag)
		{
			Decision.bAllowed = false;
			Decision.ReasonText = FString::Printf(
				TEXT("Action lock ativo: %s (source=%s action=%s)"),
				*Lock.LockTag.ToString(),
				*Lock.SourceId.ToString(),
				*ActionTag.ToString()
			);
			return Decision;
		}
	}

	const FName ActionId = ActionTag.GetTagName();
	const FOmniActionDefinition* Definition = DefinitionsById.Find(ActionId);
	if (!Definition || !Definition->bEnabled)
	{
		Decision.bAllowed = false;
		Decision.ReasonText = FString::Printf(TEXT("Acao nao encontrada/desabilitada: %s"), *ActionId.ToString());
		return Decision;
	}

	if (Definition->BlockedBy.HasAny(ContextTags))
	{
		Decision.bAllowed = false;
		Decision.ReasonText = FString::Printf(TEXT("Bloqueada por contexto: %s"), *Definition->BlockedBy.ToStringSimple());
		return Decision;
	}

	Decision.bAllowed = true;
	Decision.ReasonTag = AllowReasonTag;
	return Decision;
}

FOmniGateDecision UOmniActionGateSystem::StartAction(const FName ActionId)
{
	const FOmniGateDecision Decision = EvaluateStartAction(ActionId, true);
	PublishDecision(Decision, ActionId, true);
	return Decision;
}

bool UOmniActionGateSystem::AddLock(const FOmniGateLock& Lock)
{
	if (!Lock.IsValid())
	{
		return false;
	}

	ActiveLocks.Add(Lock);
	PublishTelemetry();
	return true;
}

bool UOmniActionGateSystem::RemoveLock(const FOmniGateLock& Lock)
{
	for (int32 Index = 0; Index < ActiveLocks.Num(); ++Index)
	{
		const FOmniGateLock& CurrentLock = ActiveLocks[Index];
		if (CurrentLock.LockTag == Lock.LockTag
			&& CurrentLock.SourceId == Lock.SourceId
			&& CurrentLock.Scope == Lock.Scope
			&& CurrentLock.ActionTag == Lock.ActionTag)
		{
			ActiveLocks.RemoveAt(Index);
			PublishTelemetry();
			return true;
		}
	}

	return false;
}

int32 UOmniActionGateSystem::GetActiveLockCount() const
{
	return ActiveLocks.Num();
}

bool UOmniActionGateSystem::StopAction(const FName ActionId, const FName Reason)
{
	if (!ActiveActions.Contains(ActionId))
	{
		return false;
	}

	RemoveAllActionLocks(ActionId);
	ActiveActions.Remove(ActionId);
	PublishTelemetry();
	BroadcastActionLifecycleEvent(OmniActionGate::EventOnActionEnded, ActionId, FString(), Reason);

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
	for (const FOmniGateLock& Lock : BuildSortedLocks())
	{
		if (Lock.LockTag.IsValid())
		{
			Result.AddTag(Lock.LockTag);
		}
	}
	return Result;
}

TArray<FOmniGateLock> UOmniActionGateSystem::GetActiveLockSnapshot() const
{
	return BuildSortedLocks();
}

TArray<FName> UOmniActionGateSystem::GetKnownActionIds() const
{
	TArray<FName> Result;
	DefinitionsById.GenerateKeyArray(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

FOmniGateDecision UOmniActionGateSystem::GetLastDecision() const
{
	return LastDecision;
}

bool UOmniActionGateSystem::TryLoadDefinitionsFromManifest(
	const UOmniManifest* Manifest,
	FString& OutError
)
{
	OutError.Reset();
	ResolvedProfileName.Reset();
	ResolvedProfileAssetPath.Reset();
	ResolvedLibraryAssetPath.Reset();

	if (!Manifest)
	{
		OutError = TEXT("Manifest is null.");
		return false;
	}
	const bool bStrictValidation = OmniActionGate::IsStrictValidationEnabled();

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

	FString ProfileAssetPathValue;
	if (!SystemEntry->TryGetSetting(OmniActionGate::ManifestSettingActionProfileAssetPath, ProfileAssetPathValue)
		|| ProfileAssetPathValue.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("Missing required manifest setting '%s' for SystemId '%s'. Configure this key in the manifest entry to point to a UOmniActionProfile asset."),
			*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString(),
			*RuntimeSystemId.ToString()
		);
	}
	else
	{
		ResolvedProfileAssetPath = ProfileAssetPathValue;
		const FSoftObjectPath ProfileAssetPath(ProfileAssetPathValue);
		if (ProfileAssetPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Invalid '%s' value for SystemId '%s': '%s'. Configure this setting with a valid UOmniActionProfile asset path."),
				*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString(),
				*RuntimeSystemId.ToString(),
				*ProfileAssetPathValue
			);
		}
		else
		{
			UObject* LoadedObject = ProfileAssetPath.TryLoad();
			if (!LoadedObject)
			{
				OutError = FString::Printf(
					TEXT("ActionProfile asset not found for SystemId '%s': %s. Fix: create the profile asset at this path or update manifest setting '%s'."),
					*RuntimeSystemId.ToString(),
					*ProfileAssetPath.ToString(),
					*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString()
				);
			}
			else if (UOmniActionProfile* CastedProfile = Cast<UOmniActionProfile>(LoadedObject))
			{
				LoadedProfile = CastedProfile;
				bLoadedFromAssetPath = true;
				ResolvedProfileName = GetNameSafe(CastedProfile);
			}
			else
			{
				OutError = FString::Printf(
					TEXT("Asset type mismatch for SystemId '%s': %s is '%s' (expected UOmniActionProfile). Fix: point setting '%s' to a UOmniActionProfile asset."),
					*RuntimeSystemId.ToString(),
					*ProfileAssetPath.ToString(),
					*LoadedObject->GetClass()->GetPathName(),
					*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString()
				);
			}
		}
	}

	if (!LoadedProfile)
	{
		if (OutError.IsEmpty())
		{
			OutError = FString::Printf(
				TEXT("Failed to resolve action profile for SystemId '%s' using setting '%s'."),
				*RuntimeSystemId.ToString(),
				*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString()
			);
		}
		return false;
	}

	if (bLoadedFromAssetPath)
	{
		const FSoftObjectPath ActionLibraryPath = LoadedProfile->ActionLibrary.ToSoftObjectPath();
		ResolvedLibraryAssetPath = ActionLibraryPath.ToString();
		if (ActionLibraryPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Profile '%s' for SystemId '%s' has null ActionLibrary. Fix in profile asset '%s': set ActionLibrary to a UOmniActionLibrary."),
				*GetNameSafe(LoadedProfile),
				*RuntimeSystemId.ToString(),
				ResolvedProfileAssetPath.IsEmpty() ? TEXT("<unknown>") : *ResolvedProfileAssetPath
			);
			return false;
		}

		UObject* LoadedLibraryObject = ActionLibraryPath.TryLoad();
		if (!LoadedLibraryObject)
		{
			OutError = FString::Printf(
				TEXT("ActionLibrary asset not found for SystemId '%s': %s. Fix in profile '%s': assign a valid UOmniActionLibrary asset."),
				*RuntimeSystemId.ToString(),
				*ActionLibraryPath.ToString(),
				*GetNameSafe(LoadedProfile)
			);
			return false;
		}
		if (!Cast<UOmniActionLibrary>(LoadedLibraryObject))
		{
			OutError = FString::Printf(
				TEXT("ActionLibrary type mismatch for SystemId '%s': %s is '%s' (expected UOmniActionLibrary). Fix the ActionLibrary reference in profile '%s'."),
				*RuntimeSystemId.ToString(),
				*ActionLibraryPath.ToString(),
				*LoadedLibraryObject->GetClass()->GetPathName(),
				*GetNameSafe(LoadedProfile)
			);
			return false;
		}
	}

	TArray<FOmniActionDefinition> LoadedDefinitions;
	LoadedProfile->ResolveDefinitions(LoadedDefinitions);
	TArray<FString> ValidationIssues;
	const FString ProfileLabel = ResolvedProfileName.IsEmpty() ? GetNameSafe(LoadedProfile) : ResolvedProfileName;
	const FString ProfilePathLabel = ResolvedProfileAssetPath.IsEmpty() ? TEXT("<unknown>") : ResolvedProfileAssetPath;
	const FString LibraryPathLabel = ResolvedLibraryAssetPath.IsEmpty() ? TEXT("<unknown>") : ResolvedLibraryAssetPath;
	auto RecordIssue = [&ValidationIssues, bStrictValidation](const FString& Issue)
	{
		ValidationIssues.Add(Issue);
		if (bStrictValidation)
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Error,
				TEXT("[Omni][ActionGate][Validate] Falha estrutural na validacao | %s"),
				*Issue
			);
		}
		else
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Warning,
				TEXT("[Omni][ActionGate][Validate] Issue de validacao (nao estrito) | %s"),
				*Issue
			);
		}
	};

	TMap<FName, int32> ActionIdCounts;
	for (int32 DefinitionIndex = LoadedDefinitions.Num() - 1; DefinitionIndex >= 0; --DefinitionIndex)
	{
		FOmniActionDefinition& LoadedDefinition = LoadedDefinitions[DefinitionIndex];
		const int32 HumanIndex = DefinitionIndex + 1;

		if (LoadedDefinition.ActionId == NAME_None)
		{
			RecordIssue(
				FString::Printf(
					TEXT("ActionProfile '%s' contains definition #%d with empty ActionId. Fix: set ActionId in ActionLibrary '%s' or profile overrides."),
					*ProfileLabel,
					HumanIndex,
					*LibraryPathLabel
				)
			);
			if (!bStrictValidation)
			{
				LoadedDefinitions.RemoveAt(DefinitionIndex);
			}
			continue;
		}

		ActionIdCounts.FindOrAdd(LoadedDefinition.ActionId)++;

		const FString ActionIdText = LoadedDefinition.ActionId.ToString();
		if (ActionIdText.StartsWith(OmniActionGate::DisallowedActionIdPrefix, ESearchCase::CaseSensitive))
		{
			RecordIssue(
				FString::Printf(
					TEXT("ActionProfile '%s' has disallowed ActionId '%s'. Fix: rename to gameplay/system namespace (example: Movement.Sprint) in '%s'."),
					*ProfileLabel,
					*ActionIdText,
					*LibraryPathLabel
				)
			);
			if (!bStrictValidation)
			{
				LoadedDefinitions.RemoveAt(DefinitionIndex);
			}
			continue;
		}

		FGameplayTagContainer SanitizedBlockedBy;
		bool bHadInvalidBlockedBy = false;
		for (const FGameplayTag& Tag : LoadedDefinition.BlockedBy)
		{
			if (Tag.IsValid())
			{
				SanitizedBlockedBy.AddTag(Tag);
			}
			else
			{
				bHadInvalidBlockedBy = true;
			}
		}
		if (bHadInvalidBlockedBy)
		{
			RecordIssue(
				FString::Printf(
					TEXT("Action '%s' in profile '%s' has invalid/empty tags in BlockedBy. Fix tags in ActionLibrary '%s'."),
					*ActionIdText,
					*ProfileLabel,
					*LibraryPathLabel
				)
			);
			if (!bStrictValidation)
			{
				LoadedDefinition.BlockedBy = MoveTemp(SanitizedBlockedBy);
			}
		}

		FGameplayTagContainer SanitizedAppliesLocks;
		bool bHadInvalidLocks = false;
		for (const FGameplayTag& Tag : LoadedDefinition.AppliesLocks)
		{
			if (Tag.IsValid())
			{
				SanitizedAppliesLocks.AddTag(Tag);
			}
			else
			{
				bHadInvalidLocks = true;
			}
		}
		if (bHadInvalidLocks)
		{
			RecordIssue(
				FString::Printf(
					TEXT("Action '%s' in profile '%s' has invalid/empty tags in AppliesLocks. Fix tags in ActionLibrary '%s'."),
					*ActionIdText,
					*ProfileLabel,
					*LibraryPathLabel
				)
			);
			if (!bStrictValidation)
			{
				LoadedDefinition.AppliesLocks = MoveTemp(SanitizedAppliesLocks);
			}
		}

		for (int32 CancelIndex = LoadedDefinition.Cancels.Num() - 1; CancelIndex >= 0; --CancelIndex)
		{
			if (LoadedDefinition.Cancels[CancelIndex] != NAME_None)
			{
				continue;
			}

			RecordIssue(
				FString::Printf(
					TEXT("Action '%s' in profile '%s' has empty reference in Cancels. Fix the Cancels list in ActionLibrary '%s'."),
					*ActionIdText,
					*ProfileLabel,
					*LibraryPathLabel
				)
			);
			if (!bStrictValidation)
			{
				LoadedDefinition.Cancels.RemoveAt(CancelIndex);
			}
		}
	}

	TArray<FName> SortedActionIds;
	ActionIdCounts.GenerateKeyArray(SortedActionIds);
	SortedActionIds.Sort(FNameLexicalLess());

	for (const FName& ActionId : SortedActionIds)
	{
		const int32 Count = ActionIdCounts.FindChecked(ActionId);
		if (Count <= 1)
		{
			continue;
		}

		RecordIssue(
			FString::Printf(
				TEXT("ActionProfile '%s' has duplicate ActionId '%s' (%dx). Fix duplicates in ActionLibrary '%s' and/or profile overrides."),
				*ProfileLabel,
				*ActionId.ToString(),
				Count,
				*LibraryPathLabel
			)
		);
	}

	TSet<FName> KnownActionIds;
	for (const FOmniActionDefinition& LoadedDefinition : LoadedDefinitions)
	{
		if (LoadedDefinition.ActionId != NAME_None)
		{
			KnownActionIds.Add(LoadedDefinition.ActionId);
		}
	}

	for (FOmniActionDefinition& LoadedDefinition : LoadedDefinitions)
	{
		if (LoadedDefinition.ActionId == NAME_None)
		{
			continue;
		}

		const FString ActionIdText = LoadedDefinition.ActionId.ToString();
		for (int32 CancelIndex = LoadedDefinition.Cancels.Num() - 1; CancelIndex >= 0; --CancelIndex)
		{
			const FName ReferencedActionId = LoadedDefinition.Cancels[CancelIndex];
			if (ReferencedActionId == NAME_None || KnownActionIds.Contains(ReferencedActionId))
			{
				continue;
			}

			RecordIssue(
				FString::Printf(
					TEXT("Action '%s' in profile '%s' references unknown action '%s' in Cancels. Fix: add '%s' to ActionLibrary '%s' or remove the reference."),
					*ActionIdText,
					*ProfileLabel,
					*ReferencedActionId.ToString(),
					*ReferencedActionId.ToString(),
					*LibraryPathLabel
				)
			);
			if (!bStrictValidation)
			{
				LoadedDefinition.Cancels.RemoveAt(CancelIndex);
			}
		}
	}

	if (bStrictValidation && ValidationIssues.Num() > 0)
	{
		const int32 MaxIssuesToShow = 3;
		TArray<FString> PreviewIssues;
		for (int32 Index = 0; Index < FMath::Min(MaxIssuesToShow, ValidationIssues.Num()); ++Index)
		{
			PreviewIssues.Add(ValidationIssues[Index]);
		}
		const FString ExtraIssuesSuffix = ValidationIssues.Num() > MaxIssuesToShow
			? FString::Printf(TEXT(" (+%d more issue(s))"), ValidationIssues.Num() - MaxIssuesToShow)
			: TEXT("");
		OutError = FString::Printf(
			TEXT("ActionProfile validation failed for SystemId '%s' [Profile=%s Path=%s Setting=%s]. %s%s"),
			*RuntimeSystemId.ToString(),
			*ProfileLabel,
			*ProfilePathLabel,
			*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString(),
			*FString::Join(PreviewIssues, TEXT(" | ")),
			*ExtraIssuesSuffix
		);
		return false;
	}

	if (LoadedDefinitions.Num() == 0)
	{
		OutError = FString::Printf(
			TEXT("Action profile '%s' resolved zero valid definitions for SystemId '%s'. Fix ActionLibrary '%s' and profile overrides."),
			*ProfileLabel,
			*RuntimeSystemId.ToString(),
			*LibraryPathLabel
		);
		return false;
	}

	DefaultDefinitions = MoveTemp(LoadedDefinitions);
	if (bLoadedFromAssetPath)
	{
		UE_LOG(
			LogOmniActionGateSystem,
			Log,
			TEXT("[Omni][ActionGate][Config] Definitions carregadas de profile=%s | count=%d strict=%s"),
			*ProfileLabel,
			DefaultDefinitions.Num(),
			bStrictValidation ? TEXT("true") : TEXT("false")
		);
		if (!bStrictValidation && ValidationIssues.Num() > 0)
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Warning,
				TEXT("[Omni][ActionGate][Config] Profile '%s' carregado com %d issue(s) | Corrija ActionLibrary para remover sanitizacao em runtime"),
				*ProfileLabel,
				ValidationIssues.Num()
			);
		}
	}
	else
	{
		OutError = FString::Printf(
			TEXT("Action definitions for SystemId '%s' were not loaded from an asset-backed profile."),
			*RuntimeSystemId.ToString()
		);
		return false;
	}

	OutError.Reset();
	return true;
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
				TEXT("[Omni][ActionGate][Config] ActionId duplicado: %s | Corrija duplicidade no ActionLibrary/profile"),
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
	return Context;
}

FOmniGateDecision UOmniActionGateSystem::EvaluateStartAction(const FName ActionId, const bool bApplyChanges)
{
	FOmniGateDecision Decision;
	Decision.ReasonTag = DenyReasonTag;

	if (!bInitialized || ActionId == NAME_None)
	{
		Decision.bAllowed = false;
		Decision.ReasonText = TEXT("Sistema nao inicializado ou ActionId invalido.");
		return Decision;
	}

	const FGameplayTag ActionTag = ResolveActionTag(ActionId);
	if (!ActionTag.IsValid())
	{
		Decision.bAllowed = false;
		Decision.ReasonText = FString::Printf(
			TEXT("ActionId sem gameplay tag registrado: %s"),
			*ActionId.ToString()
		);
		return Decision;
	}

	const FOmniActionDefinition* Definition = DefinitionsById.Find(ActionId);
	if (!Definition || !Definition->bEnabled)
	{
		Decision.bAllowed = false;
		Decision.ReasonText = FString::Printf(TEXT("Acao desconhecida/desabilitada: %s"), *ActionId.ToString());
		return Decision;
	}

	const bool bAlreadyActive = ActiveActions.Contains(ActionId);
	if (bAlreadyActive)
	{
		if (Definition->Policy == EOmniActionPolicy::DenyIfActive)
		{
			Decision.bAllowed = false;
			Decision.ReasonText = TEXT("Acao ja ativa (policy deny).");
			return Decision;
		}

		if (Definition->Policy == EOmniActionPolicy::SucceedIfActive)
		{
			Decision.bAllowed = true;
			Decision.ReasonTag = AllowReasonTag;
			Decision.ReasonText = TEXT("Acao ja ativa (policy succeed).");
			return Decision;
		}

		if (Definition->Policy == EOmniActionPolicy::RestartIfActive && bApplyChanges)
		{
			StopAction(ActionId, TEXT("RestartPolicy"));
		}
	}

	const FGameplayTagContainer BlockingContext = BuildCurrentBlockingContext();
	Decision = Can(ActionTag, BlockingContext);
	if (!Decision.bAllowed)
	{
		return Decision;
	}

	if (!bApplyChanges)
	{
		return Decision;
	}

	for (const FName ActionToCancel : Definition->Cancels)
	{
		if (ActionToCancel == NAME_None || !ActiveActions.Contains(ActionToCancel))
		{
			continue;
		}

		StopAction(ActionToCancel, ActionId);
	}

	ActiveActions.Add(ActionId);
	AddActionLocks(*Definition);
	BroadcastActionLifecycleEvent(OmniActionGate::EventOnActionStarted, ActionId);
	return Decision;
}

FGameplayTag UOmniActionGateSystem::ResolveActionTag(const FName ActionId) const
{
	if (ActionId == NAME_None)
	{
		return FGameplayTag();
	}

	return FGameplayTag::RequestGameplayTag(ActionId, false);
}

void UOmniActionGateSystem::RemoveAllActionLocks(const FName ActionId)
{
	TArray<FOmniGateLock> LocksToRemove;
	if (const TArray<FOmniGateLock>* FoundLocks = LocksByAction.Find(ActionId))
	{
		LocksToRemove = *FoundLocks;
	}
	LocksByAction.Remove(ActionId);

	for (const FOmniGateLock& Lock : LocksToRemove)
	{
		RemoveLock(Lock);
	}
}

TArray<FOmniGateLock> UOmniActionGateSystem::BuildSortedLocks() const
{
	TArray<FOmniGateLock> Result = ActiveLocks;
	Algo::StableSort(
		Result,
		[this](const FOmniGateLock& Left, const FOmniGateLock& Right)
		{
			if (Left.Scope != Right.Scope)
			{
				return IsScopeLess(Left.Scope, Right.Scope);
			}
			if (Left.LockTag != Right.LockTag)
			{
				return Left.LockTag.GetTagName().LexicalLess(Right.LockTag.GetTagName());
			}
			if (Left.ActionTag != Right.ActionTag)
			{
				return Left.ActionTag.GetTagName().LexicalLess(Right.ActionTag.GetTagName());
			}
			return Left.SourceId.LexicalLess(Right.SourceId);
		}
	);
	return Result;
}

bool UOmniActionGateSystem::IsScopeLess(const EOmniActionGateLockScope Left, const EOmniActionGateLockScope Right)
{
	if (Left == Right)
	{
		return false;
	}

	return Left == EOmniActionGateLockScope::Global;
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
	const FGameplayTag ActionTag = ResolveActionTag(Definition.ActionId);
	if (!ActionTag.IsValid())
	{
		return;
	}

	TArray<FOmniGateLock> AppliedLocks;
	for (const FGameplayTag& LockTag : Definition.AppliesLocks)
	{
		if (!LockTag.IsValid())
		{
			continue;
		}

		FOmniGateLock Lock;
		Lock.LockTag = LockTag;
		Lock.SourceId = Definition.ActionId;
		Lock.Scope = EOmniActionGateLockScope::Action;
		Lock.ActionTag = ActionTag;
		if (AddLock(Lock))
		{
			AppliedLocks.Add(Lock);
		}
	}

	LocksByAction.Add(Definition.ActionId, MoveTemp(AppliedLocks));
}

void UOmniActionGateSystem::BroadcastActionLifecycleEvent(
	const FName EventName,
	const FName ActionId,
	const FString& Reason,
	const FName EndReason
)
{
	if (!Registry.IsValid() || EventName == NAME_None || ActionId == NAME_None)
	{
		return;
	}

	FOmniEventMessage Event;
	Event.SourceSystem = OmniActionGate::SystemId;
	Event.EventName = EventName;
	Event.SetPayloadValue(TEXT("ActionId"), ActionId.ToString());
	if (!Reason.IsEmpty())
	{
		Event.SetPayloadValue(TEXT("Reason"), Reason);
	}
	if (EndReason != NAME_None)
	{
		Event.SetPayloadValue(TEXT("EndReason"), EndReason.ToString());
	}

	Registry->BroadcastEvent(Event);
}

void UOmniActionGateSystem::PublishTelemetry()
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("ActionGate.KnownActions"), FString::FromInt(DefinitionsById.Num()));
	DebugSubsystem->SetMetric(TEXT("ActionGate.ActiveActions"), FString::FromInt(ActiveActions.Num()));
	DebugSubsystem->SetMetric(TEXT("ActionGate.ActiveLocks"), FString::FromInt(ActiveLocks.Num()));
}

void UOmniActionGateSystem::PublishDecision(const FOmniGateDecision& Decision, const FName ActionId, const bool bEmitLogEntry)
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
		Event.SetPayloadValue(TEXT("ActionId"), ActionId.ToString());
		Event.SetPayloadValue(TEXT("ReasonTag"), Decision.ReasonTag.ToString());
		Event.SetPayloadValue(TEXT("ReasonText"), Decision.ReasonText);
		Registry->BroadcastEvent(Event);
	}

	if (!bEmitLogEntry)
	{
		return;
	}

	if (!Decision.bAllowed)
	{
		BroadcastActionLifecycleEvent(OmniActionGate::EventOnActionDenied, ActionId, Decision.ReasonTag.ToString());
	}

	if (Decision.bAllowed)
	{
		DebugSubsystem->LogEvent(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("ALLOW %s (%s)"), *ActionId.ToString(), *Decision.ReasonTag.ToString()),
			OmniActionGate::SourceName
		);
	}
	else
	{
		DebugSubsystem->LogWarning(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("DENY %s (%s)"), *ActionId.ToString(), *Decision.ReasonTag.ToString()),
			OmniActionGate::SourceName
		);
	}
}
