#include "Systems/Movement/OmniMovementSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Library/OmniMovementLibrary.h"
#include "Manifest/OmniManifest.h"
#include "Profile/OmniMovementProfile.h"
#include "Systems/OmniClockSubsystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniMovementSystem, Log, All);

namespace OmniMovement
{
	static const FName CategoryName(TEXT("Movement"));
	static const FName SourceName(TEXT("MovementSystem"));
	static const FName SystemId(TEXT("Movement"));
	static const FName ActionGateSystemId(TEXT("ActionGate"));
	static const FName StatusSystemId(TEXT("Status"));
	static const FName ManifestSettingMovementProfileAssetPath(TEXT("MovementProfileAssetPath"));
	static const FName ManifestSettingMovementProfileClassPath(TEXT("MovementProfileClassPath"));
	static const TCHAR* DefaultMovementProfileAssetPath = TEXT("/Game/Data/Movement/DA_Omni_MovementProfile_Default.DA_Omni_MovementProfile_Default");
	static const FName DebugMetricProfileMovement(TEXT("Omni.Profile.Movement"));
}

FName UOmniMovementSystem::GetSystemId_Implementation() const
{
	return TEXT("Movement");
}

TArray<FName> UOmniMovementSystem::GetDependencies_Implementation() const
{
	return { OmniMovement::ActionGateSystemId, OmniMovement::StatusSystemId };
}

void UOmniMovementSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);
	SetInitializationResult(false);

	Registry = Cast<UOmniSystemRegistrySubsystem>(WorldContextObject);
	if (Registry.IsValid())
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>();
			ClockSubsystem = GameInstance->GetSubsystem<UOmniClockSubsystem>();
		}
	}
	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniMovement::DebugMetricProfileMovement, TEXT("Pending"));
	}

	RuntimeSettings = FOmniMovementSettings();
	const bool bDevDefaultsEnabled = Registry.IsValid() && Registry->IsDevDefaultsEnabled();
	FString LoadError;
	if (!TryLoadSettingsFromManifest(Manifest, bDevDefaultsEnabled, LoadError))
	{
		if (!bDevDefaultsEnabled)
		{
			UE_LOG(LogOmniMovementSystem, Error, TEXT("Fail-fast [SystemId=%s]: %s"), *GetSystemId().ToString(), *LoadError);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->SetMetric(OmniMovement::DebugMetricProfileMovement, TEXT("Failed"));
				DebugSubsystem->LogError(OmniMovement::CategoryName, LoadError, OmniMovement::SourceName);
			}
			return;
		}

		if (!BuildDevFallbackSettings())
		{
			const FString FallbackError = LoadError.IsEmpty()
				? TEXT("DEV defaults enabled, but Movement fallback failed to resolve settings.")
				: FString::Printf(TEXT("%s | DEV defaults fallback also failed."), *LoadError);
			UE_LOG(LogOmniMovementSystem, Error, TEXT("Fail-fast [SystemId=%s]: %s"), *GetSystemId().ToString(), *FallbackError);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->SetMetric(OmniMovement::DebugMetricProfileMovement, TEXT("Failed"));
				DebugSubsystem->LogError(OmniMovement::CategoryName, FallbackError, OmniMovement::SourceName);
			}
			return;
		}
	}

	bSprintRequested = false;
	bIsSprinting = false;
	NextStartAttemptWorldTime = 0.0f;
	AutoSprintRemainingSeconds = 0.0f;
	PublishTelemetry();
	SetInitializationResult(true);
	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniMovement::DebugMetricProfileMovement, TEXT("Loaded"));
	}

	UE_LOG(LogOmniMovementSystem, Log, TEXT("Movement system initialized. Manifest=%s"), *GetNameSafe(Manifest));
}

void UOmniMovementSystem::ShutdownSystem_Implementation()
{
	StopSprinting(TEXT("Shutdown"));

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("Movement.SprintRequested"));
		DebugSubsystem->RemoveMetric(TEXT("Movement.IsSprinting"));
		DebugSubsystem->RemoveMetric(TEXT("Movement.AutoSprintRemaining"));
		DebugSubsystem->RemoveMetric(OmniMovement::DebugMetricProfileMovement);
		DebugSubsystem->LogEvent(OmniMovement::CategoryName, TEXT("Movement finalizado"), OmniMovement::SourceName);
	}

	DebugSubsystem.Reset();
	ClockSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniMovementSystem, Log, TEXT("Movement system shutdown."));
}

bool UOmniMovementSystem::IsTickEnabled_Implementation() const
{
	return true;
}

void UOmniMovementSystem::TickSystem_Implementation(const float DeltaTime)
{
	if (!Registry.IsValid())
	{
		return;
	}

	const double WorldTime = GetNowSeconds();

	if (RuntimeSettings.bUseKeyboardShiftAsSprintRequest)
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
			{
				const bool bShiftDown = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
				if (bShiftDown != bSprintRequested)
				{
					SetSprintRequested(bShiftDown);
				}
			}
		}
	}

	if (AutoSprintRemainingSeconds > 0.0f)
	{
		AutoSprintRemainingSeconds = FMath::Max(0.0f, AutoSprintRemainingSeconds - DeltaTime);
		if (AutoSprintRemainingSeconds <= KINDA_SMALL_NUMBER)
		{
			SetSprintRequested(false);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->LogEvent(OmniMovement::CategoryName, TEXT("AutoSprint finalizado"), OmniMovement::SourceName);
			}
		}
	}

	if (bSprintRequested && !bIsSprinting && WorldTime >= NextStartAttemptWorldTime)
	{
		StartSprinting();
	}

	if (!bSprintRequested && bIsSprinting)
	{
		StopSprinting(TEXT("Released"));
	}

	if (bIsSprinting && QueryStatusIsExhausted())
	{
		StopSprinting(TEXT("Exhausted"));
	}

	PublishTelemetry();
}

void UOmniMovementSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);

	if (Event.SourceSystem == OmniMovement::StatusSystemId && Event.EventName == OmniMessageSchema::EventExhausted)
	{
		if (bIsSprinting)
		{
			StopSprinting(TEXT("ExhaustedEvent"));
		}
	}
}

void UOmniMovementSystem::SetSprintRequested(const bool bRequested)
{
	if (bSprintRequested == bRequested)
	{
		return;
	}

	bSprintRequested = bRequested;
	if (!bSprintRequested)
	{
		AutoSprintRemainingSeconds = 0.0f;
	}

	PublishTelemetry();
}

void UOmniMovementSystem::ToggleSprintRequested()
{
	SetSprintRequested(!bSprintRequested);
}

void UOmniMovementSystem::StartAutoSprint(const float DurationSeconds)
{
	AutoSprintRemainingSeconds = FMath::Max(0.0f, DurationSeconds);
	if (AutoSprintRemainingSeconds > 0.0f)
	{
		SetSprintRequested(true);
	}
}

bool UOmniMovementSystem::IsSprintRequested() const
{
	return bSprintRequested;
}

bool UOmniMovementSystem::IsSprinting() const
{
	return bIsSprinting;
}

float UOmniMovementSystem::GetAutoSprintRemainingSeconds() const
{
	return AutoSprintRemainingSeconds;
}

void UOmniMovementSystem::StartSprinting()
{
	FString DenyReason;
	if (!QueryCanStartSprint(&DenyReason))
	{
		NextStartAttemptWorldTime = GetNowSeconds() + RuntimeSettings.FailedRetryIntervalSeconds;
		return;
	}

	if (!DispatchStartSprint())
	{
		NextStartAttemptWorldTime = GetNowSeconds() + RuntimeSettings.FailedRetryIntervalSeconds;
		return;
	}

	bIsSprinting = true;
	NextStartAttemptWorldTime = 0.0f;
	DispatchStatusSprinting(true);

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->LogEvent(OmniMovement::CategoryName, TEXT("Sprint iniciada"), OmniMovement::SourceName);
	}
}

void UOmniMovementSystem::StopSprinting(const FName Reason)
{
	if (!bIsSprinting)
	{
		DispatchStatusSprinting(false);
		return;
	}

	DispatchStopSprint(Reason);
	bIsSprinting = false;
	DispatchStatusSprinting(false);

	if (DebugSubsystem.IsValid())
	{
		const FString ReasonText = Reason == NAME_None ? TEXT("Unknown") : Reason.ToString();
		DebugSubsystem->LogEvent(
			OmniMovement::CategoryName,
			FString::Printf(TEXT("Sprint parada (%s)"), *ReasonText),
			OmniMovement::SourceName
		);
	}
}

void UOmniMovementSystem::PublishTelemetry() const
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("Movement.SprintRequested"), bSprintRequested ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(TEXT("Movement.IsSprinting"), bIsSprinting ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(TEXT("Movement.AutoSprintRemaining"), FString::Printf(TEXT("%.1fs"), AutoSprintRemainingSeconds));
}

bool UOmniMovementSystem::TryLoadSettingsFromManifest(
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

	UOmniMovementProfile* LoadedProfile = nullptr;
	bool bLoadedFromAssetPath = false;
	bool bLoadedFromClassPath = false;

	FString ProfileAssetPathValue;
	if (!SystemEntry->TryGetSetting(OmniMovement::ManifestSettingMovementProfileAssetPath, ProfileAssetPathValue)
		|| ProfileAssetPathValue.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("Missing required manifest setting '%s' for SystemId '%s'. Expected path: %s. Recommendation: create a UOmniMovementProfile asset at the expected path and assign a UOmniMovementLibrary."),
			*OmniMovement::ManifestSettingMovementProfileAssetPath.ToString(),
			*RuntimeSystemId.ToString(),
			OmniMovement::DefaultMovementProfileAssetPath
		);
	}
	else
	{
		const FSoftObjectPath ProfileAssetPath(ProfileAssetPathValue);
		if (ProfileAssetPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Invalid '%s' value for SystemId '%s': '%s'. Recommendation: set a valid UOmniMovementProfile asset path (expected: %s)."),
				*OmniMovement::ManifestSettingMovementProfileAssetPath.ToString(),
				*RuntimeSystemId.ToString(),
				*ProfileAssetPathValue,
				OmniMovement::DefaultMovementProfileAssetPath
			);
		}
		else
		{
			UObject* LoadedObject = ProfileAssetPath.TryLoad();
			if (!LoadedObject)
			{
				OutError = FString::Printf(
					TEXT("Movement profile asset not found for SystemId '%s': %s. Recommendation: create a UOmniMovementProfile asset at this path and assign a UOmniMovementLibrary."),
					*RuntimeSystemId.ToString(),
					*ProfileAssetPath.ToString()
				);
			}
			else if (UOmniMovementProfile* CastedProfile = Cast<UOmniMovementProfile>(LoadedObject))
			{
				LoadedProfile = CastedProfile;
				bLoadedFromAssetPath = true;
			}
			else
			{
				OutError = FString::Printf(
					TEXT("Asset type mismatch for SystemId '%s': %s is '%s' (expected UOmniMovementProfile)."),
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
		if (SystemEntry->TryGetSetting(OmniMovement::ManifestSettingMovementProfileClassPath, ProfileClassPathValue))
		{
			const FSoftClassPath ProfileClassPath(ProfileClassPathValue);
			if (!ProfileClassPath.IsNull())
			{
				UClass* LoadedClass = ProfileClassPath.TryLoadClass<UOmniMovementProfile>();
				if (!LoadedClass || !LoadedClass->IsChildOf(UOmniMovementProfile::StaticClass()))
				{
					if (OutError.IsEmpty())
					{
						OutError = FString::Printf(
							TEXT("Movement profile class fallback invalid for SystemId '%s': %s."),
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
					LoadedProfile = NewObject<UOmniMovementProfile>(this, LoadedClass, NAME_None, RF_Transient);
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
				TEXT("Failed to resolve movement profile for SystemId '%s'. Expected path: %s."),
				*RuntimeSystemId.ToString(),
				OmniMovement::DefaultMovementProfileAssetPath
			);
		}
		return false;
	}

	if (bLoadedFromAssetPath)
	{
		const FSoftObjectPath MovementLibraryPath = LoadedProfile->MovementLibrary.ToSoftObjectPath();
		if (MovementLibraryPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Profile '%s' for SystemId '%s' has null MovementLibrary. Recommendation: assign a UOmniMovementLibrary asset."),
				*GetNameSafe(LoadedProfile),
				*RuntimeSystemId.ToString()
			);
			return false;
		}

		UObject* LoadedLibraryObject = MovementLibraryPath.TryLoad();
		if (!LoadedLibraryObject)
		{
			OutError = FString::Printf(
				TEXT("MovementLibrary not found for SystemId '%s': %s. Recommendation: create a UOmniMovementLibrary asset and assign it in profile '%s'."),
				*RuntimeSystemId.ToString(),
				*MovementLibraryPath.ToString(),
				*GetNameSafe(LoadedProfile)
			);
			return false;
		}
		if (!Cast<UOmniMovementLibrary>(LoadedLibraryObject))
		{
			OutError = FString::Printf(
				TEXT("MovementLibrary type mismatch for SystemId '%s': %s is '%s' (expected UOmniMovementLibrary)."),
				*RuntimeSystemId.ToString(),
				*MovementLibraryPath.ToString(),
				*LoadedLibraryObject->GetClass()->GetPathName()
			);
			return false;
		}
	}

	FOmniMovementSettings LoadedSettings;
	if (!LoadedProfile->ResolveSettings(LoadedSettings))
	{
		OutError = FString::Printf(
			TEXT("Movement profile '%s' has no usable configuration for SystemId '%s'. Recommendation: configure MovementLibrary and valid settings."),
			*GetNameSafe(LoadedProfile),
			*RuntimeSystemId.ToString()
		);
		return false;
	}

	FString ValidationError;
	if (!LoadedSettings.IsValid(ValidationError))
	{
		OutError = FString::Printf(
			TEXT("Movement settings invalid for SystemId '%s' in profile '%s': %s"),
			*RuntimeSystemId.ToString(),
			*GetNameSafe(LoadedProfile),
			*ValidationError
		);
		return false;
	}

	RuntimeSettings = MoveTemp(LoadedSettings);
	if (bLoadedFromClassPath || LoadedProfile->GetClass()->IsChildOf(UOmniDevMovementProfile::StaticClass()))
	{
		UE_LOG(
			LogOmniMovementSystem,
			Warning,
			TEXT("Movement settings loaded from DEV_FALLBACK profile class: %s"),
			*LoadedProfile->GetClass()->GetPathName()
		);
	}
	else
	{
		UE_LOG(
			LogOmniMovementSystem,
			Log,
			TEXT("Movement settings loaded from profile: %s"),
			*GetNameSafe(LoadedProfile)
		);
	}

	OutError.Reset();
	return true;
}

bool UOmniMovementSystem::BuildDevFallbackSettings()
{
	static bool bWarnedDevDefaultsThisSession = false;
	if (!bWarnedDevDefaultsThisSession)
	{
		UE_LOG(LogOmniMovementSystem, Warning, TEXT("DEV_DEFAULTS ACTIVE: Movement is using fallback settings."));
		bWarnedDevDefaultsThisSession = true;
	}

	UOmniDevMovementProfile* DevProfile = NewObject<UOmniDevMovementProfile>(this, NAME_None, RF_Transient);
	if (DevProfile)
	{
		FOmniMovementSettings LoadedSettings;
		if (DevProfile->ResolveSettings(LoadedSettings))
		{
			RuntimeSettings = MoveTemp(LoadedSettings);
			UE_LOG(LogOmniMovementSystem, Warning, TEXT("Movement settings loaded from DEV_FALLBACK profile instance."));
			return true;
		}
	}

	RuntimeSettings = FOmniMovementSettings();
	UE_LOG(LogOmniMovementSystem, Warning, TEXT("Movement settings loaded from DEV_FALLBACK inline C++."));
	return true;
}

double UOmniMovementSystem::GetNowSeconds() const
{
	if (ClockSubsystem.IsValid())
	{
		return ClockSubsystem->GetSimTime();
	}
	if (Registry.IsValid() && Registry->GetWorld())
	{
		return Registry->GetWorld()->GetTimeSeconds();
	}
	return 0.0;
}

bool UOmniMovementSystem::QueryStatusIsExhausted() const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniIsExhaustedQuerySchema RequestSchema;
	RequestSchema.SourceSystem = OmniMovement::SystemId;
	FOmniQueryMessage Query = FOmniIsExhaustedQuerySchema::ToMessage(RequestSchema);

	if (!Registry->ExecuteQuery(Query) || !Query.bSuccess)
	{
		return false;
	}

	FOmniIsExhaustedQuerySchema ResponseSchema;
	FString ParseError;
	if (!FOmniIsExhaustedQuerySchema::TryFromMessage(Query, ResponseSchema, ParseError))
	{
		return false;
	}

	return ResponseSchema.bExhausted;
}

void UOmniMovementSystem::DispatchStatusSprinting(const bool bSprinting) const
{
	if (!Registry.IsValid())
	{
		return;
	}

	FOmniSetSprintingCommandSchema CommandSchema;
	CommandSchema.SourceSystem = OmniMovement::SystemId;
	CommandSchema.bSprinting = bSprinting;
	Registry->DispatchCommand(FOmniSetSprintingCommandSchema::ToMessage(CommandSchema));
}

bool UOmniMovementSystem::QueryCanStartSprint(FString* OutReason) const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniCanStartActionQuerySchema RequestSchema;
	RequestSchema.SourceSystem = OmniMovement::SystemId;
	RequestSchema.ActionId = RuntimeSettings.SprintActionId;
	FOmniQueryMessage Query = FOmniCanStartActionQuerySchema::ToMessage(RequestSchema);

	const bool bHandled = Registry->ExecuteQuery(Query);
	if (!bHandled)
	{
		return false;
	}

	FOmniCanStartActionQuerySchema ResponseSchema;
	FString ParseError;
	if (!FOmniCanStartActionQuerySchema::TryFromMessage(Query, ResponseSchema, ParseError))
	{
		return false;
	}

	if (OutReason)
	{
		*OutReason = ResponseSchema.Reason;
	}

	return ResponseSchema.bAllowed;
}

bool UOmniMovementSystem::DispatchStartSprint() const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniStartActionCommandSchema CommandSchema;
	CommandSchema.SourceSystem = OmniMovement::SystemId;
	CommandSchema.ActionId = RuntimeSettings.SprintActionId;
	return Registry->DispatchCommand(FOmniStartActionCommandSchema::ToMessage(CommandSchema));
}

bool UOmniMovementSystem::DispatchStopSprint(const FName Reason) const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniStopActionCommandSchema CommandSchema;
	CommandSchema.SourceSystem = OmniMovement::SystemId;
	CommandSchema.ActionId = RuntimeSettings.SprintActionId;
	CommandSchema.Reason = Reason;
	return Registry->DispatchCommand(FOmniStopActionCommandSchema::ToMessage(CommandSchema));
}
