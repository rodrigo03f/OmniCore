#include "Systems/Camera/OmniCameraSystem.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagAssetInterface.h"
#include "Manifest/OmniManifest.h"
#include "Misc/PackageName.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniCameraSystem, Log, All);

namespace OmniCamera
{
	static const FName CategoryName(TEXT("Camera"));
	static const FName SourceName(TEXT("CameraSystem"));
	static const FName SystemId(TEXT("Camera"));
	static const FName StatusSystemId(TEXT("Status"));
	static const FName AttributesSystemId(TEXT("Attributes"));
	static const FName ConfigSectionCamera(TEXT("Omni.Camera"));
	static const FName ConfigKeyDefaultRig(TEXT("DefaultRig"));
	static const FName ConfigKeyRigFps(TEXT("Rig.FPS"));
	static const FName ConfigKeyRigTps(TEXT("Rig.TPS"));
	static const FName ConfigKeyRigTopDown(TEXT("Rig.TopDown"));
	static const FName ConfigKeyRigOrtho2D(TEXT("Rig.Ortho2D"));
	static const TCHAR* DefaultRigAssetPath = TEXT("/Game/Data/Camera/DA_CameraRig_TPS_Default.DA_CameraRig_TPS_Default");
	static const FName ModeFpsTagName(TEXT("Game.Camera.Mode.FPS"));
	static const FName ModeTpsTagName(TEXT("Game.Camera.Mode.TPS"));
	static const FName ModeTopDownTagName(TEXT("Game.Camera.Mode.TopDown"));
	static const FName ModeOrtho2DTagName(TEXT("Game.Camera.Mode.Ortho2D"));
	static const FName SyntheticRigName(TEXT("SyntheticDefaultRig"));
	static const FName DebugMetricProfileCamera(TEXT("Omni.Profile.Camera"));
	static const FName DebugMetricMode(TEXT("Camera.Mode"));
	static const FName DebugMetricRig(TEXT("Camera.Rig"));
	static const FName DebugMetricFov(TEXT("Camera.FOV"));
	static const FName DebugMetricOrtho(TEXT("Camera.OrthoWidth"));
	static const FName DebugMetricArm(TEXT("Camera.ArmLength"));
	static const FName DebugMetricOffset(TEXT("Camera.Offset"));
	static const FName DebugMetricFallback(TEXT("Camera.Fallback"));
	static const FName DebugMetricBackend(TEXT("Camera.BackendApplied"));
	static const FName DebugMetricContext(TEXT("Camera.ContextTags"));

	struct FResolvedRigSelection
	{
		EOmniCameraMode Mode = EOmniCameraMode::TPS;
		FGameplayTag ModeTag;
		FName RigName = NAME_None;
		FOmniCameraRigSpec RigSpec;
		bool bUsedFallback = false;
		FString FallbackReason;
	};

	static FString NormalizeRigPath(const FString& RawPath)
	{
		FString Path = RawPath;
		Path.TrimStartAndEndInline();
		if (Path.IsEmpty())
		{
			return FString();
		}
		if (Path.Contains(TEXT(".")))
		{
			return Path;
		}

		const FString AssetName = FPackageName::GetShortName(Path);
		if (AssetName.IsEmpty())
		{
			return FString();
		}
		return FString::Printf(TEXT("%s.%s"), *Path, *AssetName);
	}

	static FString ModeToString(const EOmniCameraMode Mode)
	{
		switch (Mode)
		{
		case EOmniCameraMode::FPS:
			return TEXT("FPS");
		case EOmniCameraMode::TPS:
			return TEXT("TPS");
		case EOmniCameraMode::TopDown:
			return TEXT("TopDown");
		case EOmniCameraMode::Ortho2D:
			return TEXT("Ortho2D");
		default:
			return TEXT("Unknown");
		}
	}
}

FName UOmniCameraSystem::GetSystemId_Implementation() const
{
	return OmniCamera::SystemId;
}

TArray<FName> UOmniCameraSystem::GetDependencies_Implementation() const
{
	return { OmniCamera::StatusSystemId, OmniCamera::AttributesSystemId };
}

void UOmniCameraSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);
	SetInitializationResult(false);
	ResetState();

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
		DebugSubsystem->SetMetric(OmniCamera::DebugMetricProfileCamera, TEXT("Pending"));
	}

	ResolveOfficialModeTags();
	LoadConfiguredRigs(Manifest);

	SetInitializationResult(true);
	EvaluateAndApplyRig(true);

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(
			OmniCamera::DebugMetricProfileCamera,
			bUsingSyntheticDefaultRig ? TEXT("LoadedWithSyntheticDefault") : TEXT("Loaded")
		);
	}

	UE_LOG(
		LogOmniCameraSystem,
		Log,
		TEXT("[Omni][Camera][Init] Inicializado | manifest=%s defaultRig=%s"),
		*GetNameSafe(Manifest),
		DefaultRigAsset ? *DefaultRigAsset->GetPathName() : TEXT("<synthetic>")
	);
}

void UOmniCameraSystem::ShutdownSystem_Implementation()
{
	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricProfileCamera);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricMode);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricRig);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricFov);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricOrtho);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricArm);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricOffset);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricFallback);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricBackend);
		DebugSubsystem->RemoveMetric(OmniCamera::DebugMetricContext);
		DebugSubsystem->LogEvent(OmniCamera::CategoryName, TEXT("Camera finalizado"), OmniCamera::SourceName);
	}

	ResetState();
	DebugSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniCameraSystem, Log, TEXT("[Omni][Camera][Shutdown] Concluido"));
}

bool UOmniCameraSystem::IsTickEnabled_Implementation() const
{
	return true;
}

void UOmniCameraSystem::TickSystem_Implementation(const float DeltaTime)
{
	(void)DeltaTime;
	EvaluateAndApplyRig(false);
}

void UOmniCameraSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);
	(void)Event;
}

FOmniCameraSnapshot UOmniCameraSystem::GetSnapshot() const
{
	return Snapshot;
}

void UOmniCameraSystem::ResetState()
{
	DefaultRigAsset = nullptr;
	RigsByMode.Reset();
	SyntheticFallbackRig = FOmniCameraRigSpec();
	Snapshot = FOmniCameraSnapshot();
	CachedContextTags.Reset();
	bUsingSyntheticDefaultRig = false;
	bHasAppliedRigOnce = false;
	bLastSelectionUsedFallback = false;
	bLastBackendApplied = false;
	LastFallbackReason.Reset();
	LastRigName = NAME_None;
	LastModeTag = FGameplayTag();
}

void UOmniCameraSystem::ResolveOfficialModeTags()
{
	ModeFpsTag = FGameplayTag::RequestGameplayTag(OmniCamera::ModeFpsTagName, false);
	ModeTpsTag = FGameplayTag::RequestGameplayTag(OmniCamera::ModeTpsTagName, false);
	ModeTopDownTag = FGameplayTag::RequestGameplayTag(OmniCamera::ModeTopDownTagName, false);
	ModeOrtho2DTag = FGameplayTag::RequestGameplayTag(OmniCamera::ModeOrtho2DTagName, false);
}

bool UOmniCameraSystem::LoadConfiguredRigs(const UOmniManifest* Manifest)
{
	(void)Manifest;

	FString DefaultPathValue;
	if (!TryReadRigPathFromConfig(OmniCamera::ConfigKeyDefaultRig, DefaultPathValue))
	{
		DefaultPathValue = OmniCamera::DefaultRigAssetPath;
	}

	FString LoadError;
	if (!TryLoadRigAssetFromPath(DefaultPathValue, OmniCamera::ConfigKeyDefaultRig, DefaultRigAsset, LoadError))
	{
		UE_LOG(
			LogOmniCameraSystem,
			Warning,
			TEXT("[Omni][Camera][Config] DefaultRig invalido/ausente (%s). Usando fallback sintetico | %s"),
			*DefaultPathValue,
			*LoadError
		);
		bUsingSyntheticDefaultRig = true;
	}
	else
	{
		UE_LOG(
			LogOmniCameraSystem,
			Log,
			TEXT("[Omni][Camera][Config] DefaultRig carregado: %s"),
			*DefaultRigAsset->GetPathName()
		);
	}

	struct FModeRigConfig
	{
		EOmniCameraMode Mode = EOmniCameraMode::TPS;
		FName ConfigKey = NAME_None;
	};
	const TArray<FModeRigConfig> ModeConfigs = {
		{ EOmniCameraMode::FPS, OmniCamera::ConfigKeyRigFps },
		{ EOmniCameraMode::TPS, OmniCamera::ConfigKeyRigTps },
		{ EOmniCameraMode::TopDown, OmniCamera::ConfigKeyRigTopDown },
		{ EOmniCameraMode::Ortho2D, OmniCamera::ConfigKeyRigOrtho2D }
	};

	for (const FModeRigConfig& ModeConfig : ModeConfigs)
	{
		FString RigPathValue;
		if (!TryReadRigPathFromConfig(ModeConfig.ConfigKey, RigPathValue))
		{
			continue;
		}

		TObjectPtr<UOmniCameraRigSpecDataAsset> LoadedRig = nullptr;
		FString ModeLoadError;
		if (!TryLoadRigAssetFromPath(RigPathValue, ModeConfig.ConfigKey, LoadedRig, ModeLoadError))
		{
			UE_LOG(
				LogOmniCameraSystem,
				Warning,
				TEXT("[Omni][Camera][Config] Rig por mode invalido (%s=%s). Ignorando entry | %s"),
				*ModeConfig.ConfigKey.ToString(),
				*RigPathValue,
				*ModeLoadError
			);
			continue;
		}

		RigsByMode.Add(ModeConfig.Mode, LoadedRig);
		UE_LOG(
			LogOmniCameraSystem,
			Log,
			TEXT("[Omni][Camera][Config] Rig por mode carregado: %s -> %s"),
			*OmniCamera::ModeToString(ModeConfig.Mode),
			*LoadedRig->GetPathName()
		);
	}

	return true;
}

bool UOmniCameraSystem::TryReadRigPathFromConfig(const FName ConfigKey, FString& OutPath) const
{
	OutPath.Reset();
	if (!GConfig->GetString(*OmniCamera::ConfigSectionCamera.ToString(), *ConfigKey.ToString(), OutPath, GGameIni))
	{
		return false;
	}

	OutPath.TrimStartAndEndInline();
	return !OutPath.IsEmpty();
}

bool UOmniCameraSystem::TryLoadRigAssetFromPath(
	const FString& RawPath,
	const FName SourceKey,
	TObjectPtr<UOmniCameraRigSpecDataAsset>& OutRigAsset,
	FString& OutError
) const
{
	OutRigAsset = nullptr;
	OutError.Reset();

	const FString NormalizedPath = OmniCamera::NormalizeRigPath(RawPath);
	if (NormalizedPath.IsEmpty())
	{
		OutError = FString::Printf(TEXT("Path vazio/invalido para chave '%s'."), *SourceKey.ToString());
		return false;
	}

	const FSoftObjectPath RigAssetPath(NormalizedPath);
	if (RigAssetPath.IsNull())
	{
		OutError = FString::Printf(TEXT("FSoftObjectPath invalido: %s"), *NormalizedPath);
		return false;
	}

	UObject* LoadedObject = RigAssetPath.TryLoad();
	if (!LoadedObject)
	{
		OutError = FString::Printf(TEXT("Nao foi possivel carregar asset em '%s'."), *RigAssetPath.ToString());
		return false;
	}

	UOmniCameraRigSpecDataAsset* LoadedRig = Cast<UOmniCameraRigSpecDataAsset>(LoadedObject);
	if (!LoadedRig)
	{
		OutError = FString::Printf(
			TEXT("Asset em '%s' e do tipo '%s' (esperado UOmniCameraRigSpecDataAsset)."),
			*RigAssetPath.ToString(),
			*LoadedObject->GetClass()->GetPathName()
		);
		return false;
	}

	FString ValidationError;
	if (!LoadedRig->RigSpec.IsValid(ValidationError))
	{
		OutError = FString::Printf(
			TEXT("RigSpec invalido no asset '%s': %s"),
			*LoadedRig->GetPathName(),
			*ValidationError
		);
		return false;
	}

	OutRigAsset = LoadedRig;
	return true;
}

bool UOmniCameraSystem::ResolveContextTags(FGameplayTagContainer& OutContextTags) const
{
	OutContextTags.Reset();
	QuerySystemContextTags(OutContextTags);

	APlayerController* PlayerController = nullptr;
	if (!TryGetPrimaryPlayerController(PlayerController) || !PlayerController)
	{
		return OutContextTags.Num() > 0;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return OutContextTags.Num() > 0;
	}

	const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Pawn);
	if (TagInterface)
	{
		FGameplayTagContainer OwnedTags;
		TagInterface->GetOwnedGameplayTags(OwnedTags);
		OutContextTags.AppendTags(OwnedTags);
	}

	return OutContextTags.Num() > 0;
}

bool UOmniCameraSystem::QuerySystemContextTags(FGameplayTagContainer& InOutContextTags) const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	bool bAnyQuerySucceeded = false;

	const auto AppendCsvTagsToContext = [&InOutContextTags](const FString& TagsCsv)
	{
		if (TagsCsv.IsEmpty())
		{
			return;
		}

		TArray<FString> TagStrings;
		TagsCsv.ParseIntoArray(TagStrings, TEXT(","), true);
		for (FString TagString : TagStrings)
		{
			TagString.TrimStartAndEndInline();
			const FGameplayTag ParsedTag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
			if (ParsedTag.IsValid())
			{
				InOutContextTags.AddTag(ParsedTag);
			}
		}
	};

	{
		FOmniGetStateTagsCsvQuerySchema RequestSchema;
		RequestSchema.SourceSystem = OmniCamera::SystemId;
		FOmniQueryMessage Query = FOmniGetStateTagsCsvQuerySchema::ToMessage(RequestSchema);
		if (Registry->ExecuteQuery(Query) && Query.bSuccess)
		{
			FOmniGetStateTagsCsvQuerySchema ResponseSchema;
			FString ParseError;
			if (FOmniGetStateTagsCsvQuerySchema::TryFromMessage(Query, ResponseSchema, ParseError))
			{
				bAnyQuerySucceeded = true;
				AppendCsvTagsToContext(ResponseSchema.TagsCsv);
			}
		}
	}

	{
		FOmniGetStatusTagsCsvQuerySchema RequestSchema;
		RequestSchema.SourceSystem = OmniCamera::SystemId;
		FOmniQueryMessage Query = FOmniGetStatusTagsCsvQuerySchema::ToMessage(RequestSchema);
		if (Registry->ExecuteQuery(Query) && Query.bSuccess)
		{
			FOmniGetStatusTagsCsvQuerySchema ResponseSchema;
			FString ParseError;
			if (FOmniGetStatusTagsCsvQuerySchema::TryFromMessage(Query, ResponseSchema, ParseError))
			{
				bAnyQuerySucceeded = true;
				AppendCsvTagsToContext(ResponseSchema.TagsCsv);
			}
		}
	}

	return bAnyQuerySucceeded;
}

FGameplayTag UOmniCameraSystem::ResolveModeTag(const EOmniCameraMode Mode) const
{
	switch (Mode)
	{
	case EOmniCameraMode::FPS:
		return ModeFpsTag;
	case EOmniCameraMode::TPS:
		return ModeTpsTag;
	case EOmniCameraMode::TopDown:
		return ModeTopDownTag;
	case EOmniCameraMode::Ortho2D:
		return ModeOrtho2DTag;
	default:
		return FGameplayTag();
	}
}

bool UOmniCameraSystem::TryGetPrimaryPlayerController(APlayerController*& OutPlayerController) const
{
	OutPlayerController = nullptr;
	if (!Registry.IsValid())
	{
		return false;
	}

	UGameInstance* GameInstance = Registry->GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	OutPlayerController = GameInstance->GetFirstLocalPlayerController();
	return OutPlayerController != nullptr;
}

APlayerCameraManager* UOmniCameraSystem::ResolveCameraManager(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->PlayerCameraManager;
}

UCameraComponent* UOmniCameraSystem::ResolveCameraComponent(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return nullptr;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}

	return Pawn->FindComponentByClass<UCameraComponent>();
}

USpringArmComponent* UOmniCameraSystem::ResolveSpringArmComponent(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return nullptr;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}

	return Pawn->FindComponentByClass<USpringArmComponent>();
}

bool UOmniCameraSystem::ApplyRigToBackend(const FOmniCameraRigSpec& RigSpec) const
{
	APlayerController* PlayerController = nullptr;
	if (!TryGetPrimaryPlayerController(PlayerController) || !PlayerController)
	{
		return false;
	}

	bool bAppliedAny = false;

	if (APlayerCameraManager* CameraManager = ResolveCameraManager(PlayerController))
	{
		if (RigSpec.CameraMode != EOmniCameraMode::Ortho2D)
		{
			CameraManager->SetFOV(RigSpec.FOV);
			bAppliedAny = true;
		}
	}

	USpringArmComponent* SpringArm = ResolveSpringArmComponent(PlayerController);
	if (SpringArm)
	{
		SpringArm->TargetArmLength = RigSpec.ArmLength;
		SpringArm->SocketOffset = RigSpec.Offset;
		SpringArm->bEnableCameraLag = RigSpec.bUseLag;
		SpringArm->CameraLagSpeed = RigSpec.bUseLag ? RigSpec.LagSpeed : 0.0f;
		bAppliedAny = true;
	}

	if (UCameraComponent* CameraComponent = ResolveCameraComponent(PlayerController))
	{
		if (RigSpec.CameraMode == EOmniCameraMode::Ortho2D)
		{
			CameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
			CameraComponent->OrthoWidth = RigSpec.OrthoWidth;
		}
		else
		{
			CameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
			CameraComponent->SetFieldOfView(RigSpec.FOV);
		}

		if (!SpringArm)
		{
			CameraComponent->SetRelativeLocation(RigSpec.Offset);
		}

		bAppliedAny = true;
	}

	return bAppliedAny;
}

void UOmniCameraSystem::EvaluateAndApplyRig(const bool bForceLog)
{
	OmniCamera::FResolvedRigSelection Selection;
	Selection.RigSpec = SyntheticFallbackRig;
	Selection.RigName = OmniCamera::SyntheticRigName;

	ResolveContextTags(CachedContextTags);

	struct FModeCandidate
	{
		EOmniCameraMode Mode = EOmniCameraMode::TPS;
		FGameplayTag Tag;
	};
	TArray<FModeCandidate> ActiveModes;
	if (ModeFpsTag.IsValid() && CachedContextTags.HasTagExact(ModeFpsTag))
	{
		ActiveModes.Add({ EOmniCameraMode::FPS, ModeFpsTag });
	}
	if (ModeTpsTag.IsValid() && CachedContextTags.HasTagExact(ModeTpsTag))
	{
		ActiveModes.Add({ EOmniCameraMode::TPS, ModeTpsTag });
	}
	if (ModeTopDownTag.IsValid() && CachedContextTags.HasTagExact(ModeTopDownTag))
	{
		ActiveModes.Add({ EOmniCameraMode::TopDown, ModeTopDownTag });
	}
	if (ModeOrtho2DTag.IsValid() && CachedContextTags.HasTagExact(ModeOrtho2DTag))
	{
		ActiveModes.Add({ EOmniCameraMode::Ortho2D, ModeOrtho2DTag });
	}

	TObjectPtr<UOmniCameraRigSpecDataAsset> SelectedRigAsset = nullptr;
	if (ActiveModes.Num() == 1)
	{
		Selection.Mode = ActiveModes[0].Mode;
		Selection.ModeTag = ActiveModes[0].Tag;
		if (const TObjectPtr<UOmniCameraRigSpecDataAsset>* ModeRig = RigsByMode.Find(Selection.Mode))
		{
			SelectedRigAsset = *ModeRig;
		}

		if (!SelectedRigAsset)
		{
			Selection.bUsedFallback = true;
			Selection.FallbackReason = FString::Printf(
				TEXT("Rig especifica para mode '%s' nao encontrada; usando DefaultRig."),
				*OmniCamera::ModeToString(Selection.Mode)
			);
			SelectedRigAsset = DefaultRigAsset;
		}
	}
	else
	{
		Selection.bUsedFallback = true;
		if (ActiveModes.Num() == 0)
		{
			Selection.FallbackReason = TEXT("Nenhuma Game.Camera.Mode.* ativa; usando DefaultRig.");
		}
		else
		{
			TArray<FString> ActiveModeNames;
			for (const FModeCandidate& Candidate : ActiveModes)
			{
				ActiveModeNames.Add(Candidate.Tag.ToString());
			}
			ActiveModeNames.Sort();
			Selection.FallbackReason = FString::Printf(
				TEXT("Multiplas mode tags ativas (%s); usando DefaultRig."),
				*FString::Join(ActiveModeNames, TEXT(", "))
			);
		}
		SelectedRigAsset = DefaultRigAsset;
	}

	if (SelectedRigAsset)
	{
		Selection.RigName = SelectedRigAsset->GetFName();
		Selection.RigSpec = SelectedRigAsset->RigSpec;
		Selection.Mode = Selection.RigSpec.CameraMode;
		if (!Selection.ModeTag.IsValid())
		{
			Selection.ModeTag = ResolveModeTag(Selection.Mode);
		}
	}
	else
	{
		Selection.bUsedFallback = true;
		if (Selection.FallbackReason.IsEmpty())
		{
			Selection.FallbackReason = TEXT("DefaultRig ausente; usando rig sintetico.");
		}
		Selection.Mode = Selection.RigSpec.CameraMode;
		Selection.ModeTag = ResolveModeTag(Selection.Mode);
	}

	const bool bBackendApplied = ApplyRigToBackend(Selection.RigSpec);
	Snapshot.ActiveModeTag = Selection.ModeTag;
	Snapshot.ActiveRigName = Selection.RigName;
	Snapshot.CurrentFOV = Selection.RigSpec.FOV;
	Snapshot.CurrentOrthoWidth = Selection.RigSpec.OrthoWidth;
	Snapshot.CurrentArmLength = Selection.RigSpec.ArmLength;
	Snapshot.CurrentOffset = Selection.RigSpec.Offset;
	Snapshot.bUsingFallback = Selection.bUsedFallback;

	const bool bRigChanged = (LastRigName != Selection.RigName) || (LastModeTag != Selection.ModeTag);
	const bool bFallbackChanged = (bLastSelectionUsedFallback != Selection.bUsedFallback) || (LastFallbackReason != Selection.FallbackReason);
	const bool bShouldLog = bForceLog || !bHasAppliedRigOnce || bRigChanged || bFallbackChanged || (bLastBackendApplied != bBackendApplied);

	if (Selection.bUsedFallback && (bForceLog || !bHasAppliedRigOnce || bFallbackChanged || bRigChanged))
	{
		UE_LOG(
			LogOmniCameraSystem,
			Warning,
			TEXT("[Omni][Camera][Fallback] %s"),
			Selection.FallbackReason.IsEmpty() ? TEXT("Fallback aplicado.") : *Selection.FallbackReason
		);
	}

	if (bShouldLog)
	{
		UE_LOG(
			LogOmniCameraSystem,
			Log,
			TEXT("[Omni][Camera][Apply] mode=%s rig=%s fov=%.2f ortho=%.2f arm=%.2f offset=%s backendApplied=%s"),
			Selection.ModeTag.IsValid() ? *Selection.ModeTag.ToString() : TEXT("<none>"),
			Selection.RigName == NAME_None ? TEXT("<none>") : *Selection.RigName.ToString(),
			Selection.RigSpec.FOV,
			Selection.RigSpec.OrthoWidth,
			Selection.RigSpec.ArmLength,
			*Selection.RigSpec.Offset.ToCompactString(),
			bBackendApplied ? TEXT("True") : TEXT("False")
		);
	}

	LastRigName = Selection.RigName;
	LastModeTag = Selection.ModeTag;
	bLastSelectionUsedFallback = Selection.bUsedFallback;
	LastFallbackReason = Selection.FallbackReason;
	bLastBackendApplied = bBackendApplied;
	bHasAppliedRigOnce = true;

	PublishTelemetry(bBackendApplied);
}

void UOmniCameraSystem::PublishTelemetry(const bool bBackendApplied) const
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(
		OmniCamera::DebugMetricMode,
		Snapshot.ActiveModeTag.IsValid() ? Snapshot.ActiveModeTag.ToString() : TEXT("<none>")
	);
	DebugSubsystem->SetMetric(
		OmniCamera::DebugMetricRig,
		Snapshot.ActiveRigName == NAME_None ? TEXT("<none>") : Snapshot.ActiveRigName.ToString()
	);
	DebugSubsystem->SetMetric(OmniCamera::DebugMetricFov, FString::Printf(TEXT("%.2f"), Snapshot.CurrentFOV));
	DebugSubsystem->SetMetric(OmniCamera::DebugMetricOrtho, FString::Printf(TEXT("%.2f"), Snapshot.CurrentOrthoWidth));
	DebugSubsystem->SetMetric(OmniCamera::DebugMetricArm, FString::Printf(TEXT("%.2f"), Snapshot.CurrentArmLength));
	DebugSubsystem->SetMetric(OmniCamera::DebugMetricOffset, Snapshot.CurrentOffset.ToCompactString());
	DebugSubsystem->SetMetric(OmniCamera::DebugMetricFallback, Snapshot.bUsingFallback ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(OmniCamera::DebugMetricBackend, bBackendApplied ? TEXT("True") : TEXT("False"));

	TArray<FString> ContextTagStrings;
	for (const FGameplayTag& ContextTag : CachedContextTags)
	{
		if (ContextTag.IsValid())
		{
			ContextTagStrings.Add(ContextTag.ToString());
		}
	}
	ContextTagStrings.Sort();
	DebugSubsystem->SetMetric(
		OmniCamera::DebugMetricContext,
		ContextTagStrings.Num() > 0 ? FString::Join(ContextTagStrings, TEXT(",")) : TEXT("<empty>")
	);
}
