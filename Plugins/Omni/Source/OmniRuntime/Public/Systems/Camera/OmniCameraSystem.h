#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Profile/OmniCameraRigSpecDataAsset.h"
#include "Systems/Camera/OmniCameraData.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniCameraSystem.generated.h"

class APlayerCameraManager;
class APlayerController;
class UCameraComponent;
class UOmniDebugSubsystem;
class UOmniManifest;
class UOmniSystemRegistrySubsystem;
class USpringArmComponent;

// Purpose:
// - Selecionar rig de camera via Mode tags de contexto.
// - Aplicar rig no backend Unreal (PCM/CameraComponent/SpringArm).
// - Expor snapshot deterministico para UI/debug.
// Inputs:
// - Tags de contexto agregadas de Attributes + Status + GameplayTagAssetInterface no Pawn.
// - Config de rigs em DefaultGame.ini.
// Outputs:
// - Camera aplicada no backend Unreal.
// - Snapshot publico do estado ativo.
// - Telemetria/log no padrao Omni.
// Determinism:
// - Selecao por regra fixa: 1 mode tag => rig do mode; 0/multiplas => fallback.
// - Sem blend temporal no U1.
// Failure modes:
// - Rig ausente/invalida gera fallback sintetico com log explicito.
// - Ausencia de backend (PCM/Camera) preserva snapshot e tenta reaplicar no proximo tick.
UCLASS()
class OMNIRUNTIME_API UOmniCameraSystem : public UOmniRuntimeSystem
{
	GENERATED_BODY()

public:
	virtual FName GetSystemId_Implementation() const override;
	virtual TArray<FName> GetDependencies_Implementation() const override;
	virtual void InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest) override;
	virtual void ShutdownSystem_Implementation() override;
	virtual bool IsTickEnabled_Implementation() const override;
	virtual void TickSystem_Implementation(float DeltaTime) override;
	virtual void HandleEvent_Implementation(const FOmniEventMessage& Event) override;

	UFUNCTION(BlueprintPure, Category = "Omni|Camera")
	FOmniCameraSnapshot GetSnapshot() const;

private:
	void ResetState();
	void ResolveOfficialModeTags();
	bool LoadConfiguredRigs(const UOmniManifest* Manifest);
	bool TryReadRigPathFromConfig(FName ConfigKey, FString& OutPath) const;
	bool TryLoadRigAssetFromPath(
		const FString& RawPath,
		FName SourceKey,
		TObjectPtr<UOmniCameraRigSpecDataAsset>& OutRigAsset,
		FString& OutError
	) const;
	bool ResolveContextTags(FGameplayTagContainer& OutContextTags) const;
	bool QuerySystemContextTags(FGameplayTagContainer& InOutContextTags) const;
	FGameplayTag ResolveModeTag(EOmniCameraMode Mode) const;
	bool TryGetPrimaryPlayerController(APlayerController*& OutPlayerController) const;
	APlayerCameraManager* ResolveCameraManager(APlayerController* PlayerController) const;
	UCameraComponent* ResolveCameraComponent(APlayerController* PlayerController) const;
	USpringArmComponent* ResolveSpringArmComponent(APlayerController* PlayerController) const;
	bool ApplyRigToBackend(const FOmniCameraRigSpec& RigSpec) const;
	void EvaluateAndApplyRig(bool bForceLog);
	void PublishTelemetry(bool bBackendApplied) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UOmniCameraRigSpecDataAsset> DefaultRigAsset = nullptr;

	UPROPERTY(Transient)
	TMap<EOmniCameraMode, TObjectPtr<UOmniCameraRigSpecDataAsset>> RigsByMode;

	UPROPERTY(Transient)
	FOmniCameraRigSpec SyntheticFallbackRig;

	UPROPERTY(Transient)
	FOmniCameraSnapshot Snapshot;

	UPROPERTY(Transient)
	FGameplayTagContainer CachedContextTags;

	UPROPERTY(Transient)
	bool bUsingSyntheticDefaultRig = false;

	UPROPERTY(Transient)
	bool bHasAppliedRigOnce = false;

	UPROPERTY(Transient)
	bool bLastSelectionUsedFallback = false;

	UPROPERTY(Transient)
	bool bLastBackendApplied = false;

	UPROPERTY(Transient)
	FString LastFallbackReason;

	UPROPERTY(Transient)
	FName LastRigName = NAME_None;

	UPROPERTY(Transient)
	FGameplayTag LastModeTag;

	UPROPERTY(Transient)
	FGameplayTag ModeFpsTag;

	UPROPERTY(Transient)
	FGameplayTag ModeTpsTag;

	UPROPERTY(Transient)
	FGameplayTag ModeTopDownTag;

	UPROPERTY(Transient)
	FGameplayTag ModeOrtho2DTag;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;
};
