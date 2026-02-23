#include "Systems/Status/OmniStatusSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Library/OmniStatusLibrary.h"
#include "Manifest/OmniManifest.h"
#include "Profile/OmniStatusProfile.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniStatusSystem, Log, All);

namespace OmniStatus
{
	static const FName CategoryName(TEXT("Status"));
	static const FName SourceName(TEXT("StatusSystem"));
	static const FName SystemId(TEXT("Status"));
	static const FName CommandSetSprinting(TEXT("SetSprinting"));
	static const FName CommandConsumeStamina(TEXT("ConsumeStamina"));
	static const FName CommandAddStamina(TEXT("AddStamina"));
	static const FName QueryIsExhausted(TEXT("IsExhausted"));
	static const FName QueryGetStateTagsCsv(TEXT("GetStateTagsCsv"));
	static const FName QueryGetStamina(TEXT("GetStamina"));
	static const FName ManifestSettingStatusProfileAssetPath(TEXT("StatusProfileAssetPath"));
	static const FName ManifestSettingStatusProfileClassPath(TEXT("StatusProfileClassPath"));
	static const TCHAR* DefaultStatusProfileAssetPath = TEXT("/Game/Data/Status/DA_Omni_StatusProfile_Default.DA_Omni_StatusProfile_Default");
	static const FName DebugMetricProfileStatus(TEXT("Omni.Profile.Status"));
}

FName UOmniStatusSystem::GetSystemId_Implementation() const
{
	return TEXT("Status");
}

TArray<FName> UOmniStatusSystem::GetDependencies_Implementation() const
{
	return {};
}

void UOmniStatusSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);
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
		DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Pending"));
	}

	RuntimeSettings = FOmniStatusSettings();
	const bool bDevDefaultsEnabled = Registry.IsValid() && Registry->IsDevDefaultsEnabled();
	FString LoadError;
	if (!TryLoadSettingsFromManifest(Manifest, bDevDefaultsEnabled, LoadError))
	{
		if (!bDevDefaultsEnabled)
		{
			UE_LOG(LogOmniStatusSystem, Error, TEXT("Fail-fast [SystemId=%s]: %s"), *GetSystemId().ToString(), *LoadError);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Failed"));
				DebugSubsystem->LogError(OmniStatus::CategoryName, LoadError, OmniStatus::SourceName);
			}
			return;
		}

		if (!BuildDevFallbackSettings())
		{
			const FString FallbackError = LoadError.IsEmpty()
				? TEXT("DEV defaults enabled, but Status fallback failed to resolve settings.")
				: FString::Printf(TEXT("%s | DEV defaults fallback also failed."), *LoadError);
			UE_LOG(LogOmniStatusSystem, Error, TEXT("Fail-fast [SystemId=%s]: %s"), *GetSystemId().ToString(), *FallbackError);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Failed"));
				DebugSubsystem->LogError(OmniStatus::CategoryName, FallbackError, OmniStatus::SourceName);
			}
			return;
		}
	}

	ExhaustedTag = RuntimeSettings.ExhaustedTag;
	if (!ExhaustedTag.IsValid())
	{
		ExhaustedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Exhausted"), false);
	}

	CurrentStamina = RuntimeSettings.MaxStamina;
	bExhausted = false;
	bSprinting = false;
	TimeSinceLastConsumption = RuntimeSettings.RegenDelaySeconds;
	UpdateStateTags();
	PublishTelemetry();
	SetInitializationResult(true);
	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Loaded"));
	}

	UE_LOG(LogOmniStatusSystem, Log, TEXT("Status system initialized. Manifest=%s"), *GetNameSafe(Manifest));

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->LogEvent(
			OmniStatus::CategoryName,
			FString::Printf(TEXT("Status inicializado. Stamina=%.1f"), CurrentStamina),
			OmniStatus::SourceName
		);
	}
}

void UOmniStatusSystem::ShutdownSystem_Implementation()
{
	bSprinting = false;
	bExhausted = false;
	StateTags.Reset();

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("Status.Stamina"));
		DebugSubsystem->RemoveMetric(TEXT("Status.Exhausted"));
		DebugSubsystem->RemoveMetric(TEXT("Status.Sprinting"));
		DebugSubsystem->RemoveMetric(OmniStatus::DebugMetricProfileStatus);
		DebugSubsystem->LogEvent(OmniStatus::CategoryName, TEXT("Status finalizado"), OmniStatus::SourceName);
	}

	DebugSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniStatusSystem, Log, TEXT("Status system shutdown."));
}

bool UOmniStatusSystem::IsTickEnabled_Implementation() const
{
	return true;
}

void UOmniStatusSystem::TickSystem_Implementation(const float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	if (bSprinting)
	{
		ConsumeStamina(RuntimeSettings.SprintDrainPerSecond * DeltaTime);
	}
	else
	{
		TimeSinceLastConsumption += DeltaTime;
		if (TimeSinceLastConsumption >= RuntimeSettings.RegenDelaySeconds && CurrentStamina < RuntimeSettings.MaxStamina)
		{
			AddStamina(RuntimeSettings.RegenPerSecond * DeltaTime);
		}
	}

	if (!bExhausted && CurrentStamina <= KINDA_SMALL_NUMBER)
	{
		bExhausted = true;
		UpdateStateTags();

		if (Registry.IsValid())
		{
			FOmniExhaustedEventSchema EventSchema;
			EventSchema.SourceSystem = OmniStatus::SystemId;
			Registry->BroadcastEvent(FOmniExhaustedEventSchema::ToMessage(EventSchema));
		}

		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->LogWarning(OmniStatus::CategoryName, TEXT("Entrou em estado Exhausted"), OmniStatus::SourceName);
		}
	}
	else if (bExhausted && CurrentStamina >= RuntimeSettings.ExhaustRecoverThreshold)
	{
		bExhausted = false;
		UpdateStateTags();

		if (Registry.IsValid())
		{
			FOmniExhaustedClearedEventSchema EventSchema;
			EventSchema.SourceSystem = OmniStatus::SystemId;
			Registry->BroadcastEvent(FOmniExhaustedClearedEventSchema::ToMessage(EventSchema));
		}

		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->LogEvent(OmniStatus::CategoryName, TEXT("Saiu de estado Exhausted"), OmniStatus::SourceName);
		}
	}

	PublishTelemetry();
}

bool UOmniStatusSystem::HandleCommand_Implementation(const FOmniCommandMessage& Command)
{
	if (Command.CommandName == OmniMessageSchema::CommandSetSprinting)
	{
		FOmniSetSprintingCommandSchema ParsedSchema;
		FString ParseError;
		if (!FOmniSetSprintingCommandSchema::TryFromMessage(Command, ParsedSchema, ParseError))
		{
			return false;
		}

		SetSprinting(ParsedSchema.bSprinting);
		return true;
	}

	if (Command.CommandName == OmniStatus::CommandConsumeStamina)
	{
		FString AmountValue;
		if (!Command.TryGetArgument(TEXT("Amount"), AmountValue))
		{
			return false;
		}

		float Amount = 0.0f;
		LexFromString(Amount, *AmountValue);
		ConsumeStamina(Amount);
		return true;
	}

	if (Command.CommandName == OmniStatus::CommandAddStamina)
	{
		FString AmountValue;
		if (!Command.TryGetArgument(TEXT("Amount"), AmountValue))
		{
			return false;
		}

		float Amount = 0.0f;
		LexFromString(Amount, *AmountValue);
		AddStamina(Amount);
		return true;
	}

	return Super::HandleCommand_Implementation(Command);
}

bool UOmniStatusSystem::HandleQuery_Implementation(FOmniQueryMessage& Query)
{
	if (Query.QueryName == OmniStatus::QueryIsExhausted)
	{
		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = bExhausted ? TEXT("True") : TEXT("False");
		return true;
	}

	if (Query.QueryName == OmniStatus::QueryGetStateTagsCsv)
	{
		TArray<FString> Tags;
		for (const FGameplayTag& Tag : StateTags)
		{
			if (Tag.IsValid())
			{
				Tags.Add(Tag.ToString());
			}
		}

		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = FString::Join(Tags, TEXT(","));
		return true;
	}

	if (Query.QueryName == OmniStatus::QueryGetStamina)
	{
		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = FString::Printf(TEXT("%.2f/%.2f"), CurrentStamina, RuntimeSettings.MaxStamina);
		Query.SetOutputValue(TEXT("Current"), FString::Printf(TEXT("%.2f"), CurrentStamina));
		Query.SetOutputValue(TEXT("Max"), FString::Printf(TEXT("%.2f"), RuntimeSettings.MaxStamina));
		Query.SetOutputValue(TEXT("Normalized"), FString::Printf(TEXT("%.4f"), GetStaminaNormalized()));
		return true;
	}

	return Super::HandleQuery_Implementation(Query);
}

void UOmniStatusSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);
	(void)Event;
}

float UOmniStatusSystem::GetCurrentStamina() const
{
	return CurrentStamina;
}

float UOmniStatusSystem::GetMaxStamina() const
{
	return RuntimeSettings.MaxStamina;
}

float UOmniStatusSystem::GetStaminaNormalized() const
{
	if (RuntimeSettings.MaxStamina <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return CurrentStamina / RuntimeSettings.MaxStamina;
}

bool UOmniStatusSystem::IsExhausted() const
{
	return bExhausted;
}

const FGameplayTagContainer& UOmniStatusSystem::GetStateTags() const
{
	return StateTags;
}

void UOmniStatusSystem::SetSprinting(const bool bInSprinting)
{
	if (bSprinting == bInSprinting)
	{
		return;
	}

	bSprinting = bInSprinting;
	if (!bSprinting)
	{
		TimeSinceLastConsumption = 0.0f;
	}

	PublishTelemetry();
}

void UOmniStatusSystem::ConsumeStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);
	TimeSinceLastConsumption = 0.0f;
}

void UOmniStatusSystem::AddStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentStamina = FMath::Min(RuntimeSettings.MaxStamina, CurrentStamina + Amount);
}

bool UOmniStatusSystem::TryLoadSettingsFromManifest(
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

	UOmniStatusProfile* LoadedProfile = nullptr;
	bool bLoadedFromAssetPath = false;
	bool bLoadedFromClassPath = false;

	FString ProfileAssetPathValue;
	if (!SystemEntry->TryGetSetting(OmniStatus::ManifestSettingStatusProfileAssetPath, ProfileAssetPathValue)
		|| ProfileAssetPathValue.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("Missing required manifest setting '%s' for SystemId '%s'. Expected path: %s. Recommendation: create a UOmniStatusProfile asset at the expected path and assign a UOmniStatusLibrary."),
			*OmniStatus::ManifestSettingStatusProfileAssetPath.ToString(),
			*RuntimeSystemId.ToString(),
			OmniStatus::DefaultStatusProfileAssetPath
		);
	}
	else
	{
		const FSoftObjectPath ProfileAssetPath(ProfileAssetPathValue);
		if (ProfileAssetPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Invalid '%s' value for SystemId '%s': '%s'. Recommendation: set a valid UOmniStatusProfile asset path (expected: %s)."),
				*OmniStatus::ManifestSettingStatusProfileAssetPath.ToString(),
				*RuntimeSystemId.ToString(),
				*ProfileAssetPathValue,
				OmniStatus::DefaultStatusProfileAssetPath
			);
		}
		else
		{
			UObject* LoadedObject = ProfileAssetPath.TryLoad();
			if (!LoadedObject)
			{
				OutError = FString::Printf(
					TEXT("Status profile asset not found for SystemId '%s': %s. Recommendation: create a UOmniStatusProfile asset at this path and assign a UOmniStatusLibrary."),
					*RuntimeSystemId.ToString(),
					*ProfileAssetPath.ToString()
				);
			}
			else if (UOmniStatusProfile* CastedProfile = Cast<UOmniStatusProfile>(LoadedObject))
			{
				LoadedProfile = CastedProfile;
				bLoadedFromAssetPath = true;
			}
			else
			{
				OutError = FString::Printf(
					TEXT("Asset type mismatch for SystemId '%s': %s is '%s' (expected UOmniStatusProfile)."),
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
		if (SystemEntry->TryGetSetting(OmniStatus::ManifestSettingStatusProfileClassPath, ProfileClassPathValue))
		{
			const FSoftClassPath ProfileClassPath(ProfileClassPathValue);
			if (!ProfileClassPath.IsNull())
			{
				UClass* LoadedClass = ProfileClassPath.TryLoadClass<UOmniStatusProfile>();
				if (!LoadedClass || !LoadedClass->IsChildOf(UOmniStatusProfile::StaticClass()))
				{
					if (OutError.IsEmpty())
					{
						OutError = FString::Printf(
							TEXT("Status profile class fallback invalid for SystemId '%s': %s."),
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
					LoadedProfile = NewObject<UOmniStatusProfile>(this, LoadedClass, NAME_None, RF_Transient);
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
				TEXT("Failed to resolve status profile for SystemId '%s'. Expected path: %s."),
				*RuntimeSystemId.ToString(),
				OmniStatus::DefaultStatusProfileAssetPath
			);
		}
		return false;
	}

	if (bLoadedFromAssetPath)
	{
		const FSoftObjectPath StatusLibraryPath = LoadedProfile->StatusLibrary.ToSoftObjectPath();
		if (StatusLibraryPath.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Profile '%s' for SystemId '%s' has null StatusLibrary. Recommendation: assign a UOmniStatusLibrary asset."),
				*GetNameSafe(LoadedProfile),
				*RuntimeSystemId.ToString()
			);
			return false;
		}

		UObject* LoadedLibraryObject = StatusLibraryPath.TryLoad();
		if (!LoadedLibraryObject)
		{
			OutError = FString::Printf(
				TEXT("StatusLibrary not found for SystemId '%s': %s. Recommendation: create a UOmniStatusLibrary asset and assign it in profile '%s'."),
				*RuntimeSystemId.ToString(),
				*StatusLibraryPath.ToString(),
				*GetNameSafe(LoadedProfile)
			);
			return false;
		}
		if (!Cast<UOmniStatusLibrary>(LoadedLibraryObject))
		{
			OutError = FString::Printf(
				TEXT("StatusLibrary type mismatch for SystemId '%s': %s is '%s' (expected UOmniStatusLibrary)."),
				*RuntimeSystemId.ToString(),
				*StatusLibraryPath.ToString(),
				*LoadedLibraryObject->GetClass()->GetPathName()
			);
			return false;
		}
	}

	FOmniStatusSettings LoadedSettings;
	if (!LoadedProfile->ResolveSettings(LoadedSettings))
	{
		OutError = FString::Printf(
			TEXT("Status profile '%s' has no usable configuration for SystemId '%s'. Recommendation: configure StatusLibrary and valid settings."),
			*GetNameSafe(LoadedProfile),
			*RuntimeSystemId.ToString()
		);
		return false;
	}

	FString ValidationError;
	if (!LoadedSettings.IsValid(ValidationError))
	{
		OutError = FString::Printf(
			TEXT("Status settings invalid for SystemId '%s' in profile '%s': %s"),
			*RuntimeSystemId.ToString(),
			*GetNameSafe(LoadedProfile),
			*ValidationError
		);
		return false;
	}

	RuntimeSettings = MoveTemp(LoadedSettings);
	if (bLoadedFromClassPath || LoadedProfile->GetClass()->IsChildOf(UOmniDevStatusProfile::StaticClass()))
	{
		UE_LOG(
			LogOmniStatusSystem,
			Warning,
			TEXT("Status settings loaded from DEV_FALLBACK profile class: %s"),
			*LoadedProfile->GetClass()->GetPathName()
		);
	}
	else
	{
		UE_LOG(
			LogOmniStatusSystem,
			Log,
			TEXT("Status settings loaded from profile: %s"),
			*GetNameSafe(LoadedProfile)
		);
	}

	OutError.Reset();
	return true;
}

bool UOmniStatusSystem::BuildDevFallbackSettings()
{
	static bool bWarnedDevDefaultsThisSession = false;
	if (!bWarnedDevDefaultsThisSession)
	{
		UE_LOG(LogOmniStatusSystem, Warning, TEXT("DEV_DEFAULTS ACTIVE: Status is using fallback settings."));
		bWarnedDevDefaultsThisSession = true;
	}

	UOmniDevStatusProfile* DevProfile = NewObject<UOmniDevStatusProfile>(this, NAME_None, RF_Transient);
	if (DevProfile)
	{
		FOmniStatusSettings LoadedSettings;
		if (DevProfile->ResolveSettings(LoadedSettings))
		{
			RuntimeSettings = MoveTemp(LoadedSettings);
			UE_LOG(LogOmniStatusSystem, Warning, TEXT("Status settings loaded from DEV_FALLBACK profile instance."));
			return true;
		}
	}

	RuntimeSettings = FOmniStatusSettings();
	UE_LOG(LogOmniStatusSystem, Warning, TEXT("Status settings loaded from DEV_FALLBACK inline C++."));
	return true;
}

void UOmniStatusSystem::UpdateStateTags()
{
	StateTags.Reset();
	if (bExhausted && ExhaustedTag.IsValid())
	{
		StateTags.AddTag(ExhaustedTag);
	}
}

void UOmniStatusSystem::PublishTelemetry()
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("Status.Stamina"), FString::Printf(TEXT("%.1f/%.1f"), CurrentStamina, RuntimeSettings.MaxStamina));
	DebugSubsystem->SetMetric(TEXT("Status.Exhausted"), bExhausted ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(TEXT("Status.Sprinting"), bSprinting ? TEXT("True") : TEXT("False"));
}
