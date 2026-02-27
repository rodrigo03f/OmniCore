#include "Systems/ActionGate/OmniActionGateSystem.h"

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
	static const FName ManifestSettingActionProfileAssetPath(TEXT("ActionProfileAssetPath"));
	static const TCHAR* DisallowedActionIdPrefix = TEXT("Input.");
	static const FName DebugMetricProfileAction(TEXT("Omni.Profile.Action"));
	static TAutoConsoleVariable<int32> CVarActionGateFailFast(
		TEXT("omni.actiongate.failfast"),
		-1,
		TEXT("ActionGate validation strict mode.\n-1=Auto (strict in non-shipping, non-strict in shipping)\n0=Non-strict (log + deny/sanitize)\n1=Strict fail-fast"),
		ECVF_Default
	);

	static FString DecisionToResult(const FOmniActionGateDecision& Decision)
	{
		return FString::Printf(TEXT("%s | %s"), Decision.bAllowed ? TEXT("ALLOW") : TEXT("DENY"), *Decision.Reason);
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
			UE_LOG(LogOmniActionGateSystem, Error, TEXT("%s"), *EmptyDefinitionsError);
		}
		else
		{
			UE_LOG(LogOmniActionGateSystem, Warning, TEXT("%s"), *EmptyDefinitionsError);
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
	ActiveLockRefCounts.Reset();
	LastDecision = FOmniActionGateDecision();
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
			UE_LOG(LogOmniActionGateSystem, Error, TEXT("%s"), *Issue);
		}
		else
		{
			UE_LOG(LogOmniActionGateSystem, Warning, TEXT("%s"), *Issue);
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

	for (const TPair<FName, int32>& Pair : ActionIdCounts)
	{
		if (Pair.Value <= 1)
		{
			continue;
		}

		RecordIssue(
			FString::Printf(
				TEXT("ActionProfile '%s' has duplicate ActionId '%s' (%dx). Fix duplicates in ActionLibrary '%s' and/or profile overrides."),
				*ProfileLabel,
				*Pair.Key.ToString(),
				Pair.Value,
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
			TEXT("Action definitions loaded from profile: %s (Definitions=%d, StrictValidation=%s)"),
			*ProfileLabel,
			DefaultDefinitions.Num(),
			bStrictValidation ? TEXT("true") : TEXT("false")
		);
		if (!bStrictValidation && ValidationIssues.Num() > 0)
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Warning,
				TEXT("ActionProfile '%s' loaded with %d validation issue(s). Invalid entries were sanitized/denied at runtime."),
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
		const bool bStrictValidation = OmniActionGate::IsStrictValidationEnabled();
		TArray<FName> KnownActionIds;
		DefinitionsById.GenerateKeyArray(KnownActionIds);
		KnownActionIds.Sort(FNameLexicalLess());
		const FString KnownActionsText = KnownActionIds.Num() > 0
			? FString::JoinBy(
				KnownActionIds,
				TEXT(", "),
				[](const FName KnownId)
				{
					return KnownId.ToString();
				}
			)
			: TEXT("<none>");

		const FString ProfileLabel = ResolvedProfileName.IsEmpty() ? TEXT("<unknown>") : ResolvedProfileName;
		const FString ProfilePathLabel = ResolvedProfileAssetPath.IsEmpty() ? TEXT("<unknown>") : ResolvedProfileAssetPath;
		const FString LibraryPathLabel = ResolvedLibraryAssetPath.IsEmpty() ? TEXT("<unknown>") : ResolvedLibraryAssetPath;
		const FString ActionNotFoundMessage = FString::Printf(
			TEXT("Action '%s' was requested but is not available in ActionProfile '%s'. Fix: set manifest setting '%s' (SystemId '%s') to profile '%s' and ensure ActionLibrary '%s' defines this ActionId. KnownActions=[%s]"),
			*ActionId.ToString(),
			*ProfileLabel,
			*OmniActionGate::ManifestSettingActionProfileAssetPath.ToString(),
			*GetSystemId().ToString(),
			*ProfilePathLabel,
			*LibraryPathLabel,
			*KnownActionsText
		);

		if (bStrictValidation)
		{
			ensureAlwaysMsgf(false, TEXT("%s"), *ActionNotFoundMessage);
		}
		if (bStrictValidation)
		{
			UE_LOG(LogOmniActionGateSystem, Error, TEXT("%s"), *ActionNotFoundMessage);
		}
		else
		{
			UE_LOG(LogOmniActionGateSystem, Warning, TEXT("%s"), *ActionNotFoundMessage);
		}

		Decision.bAllowed = false;
		Decision.Reason = FString::Printf(
			TEXT("Acao '%s' desconhecida/desabilitada no profile '%s'. Verifique ActionLibrary/manifest."),
			*ActionId.ToString(),
			*ProfileLabel
		);
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
