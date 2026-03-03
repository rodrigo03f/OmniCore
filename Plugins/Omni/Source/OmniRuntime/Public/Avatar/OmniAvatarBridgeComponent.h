#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "OmniAvatarBridgeComponent.generated.h"

class APawn;
class UCharacterMovementComponent;
class UOmniMovementSystem;
class UOmniSystemRegistrySubsystem;
class UOmniUIBridgeSubsystem;

// Purpose:
// - Integrar host Character/Pawn ao runtime Omni de forma canônica.
// - Produzir context tags mínimas de avatar para consumo por sistemas/bridges.
// - Aplicar output de movimento no backend CMC (MaxWalkSpeed).
// - Garantir bootstrap da HUD via bridge/config sem duplicar lógica.
// Inputs:
// - Host Pawn/Character + CMC local.
// - Snapshot/estado do MovementSystem no Registry.
// - Config [Omni.Movement] SprintMultiplier.
// Outputs:
// - Context tags de avatar (SprintRequested + Camera.Mode.TPS).
// - MaxWalkSpeed efetiva aplicada no CMC.
// - Tentativa de criação da HUD via UOmniUIBridgeSubsystem.
// Determinism:
// - EffectiveSpeed depende de BaseSpeed capturada + estado de sprint + config.
// - Sem uso de random/time no cálculo de backend.
// Failure modes:
// - Host inválido/sem CMC => fallback com log único e backend desativado.
// - Registry/Movement/HUD indisponíveis => tentativa contínua com log sem spam.
UCLASS(ClassGroup = (Omni), meta = (BlueprintSpawnableComponent))
class OMNIRUNTIME_API UOmniAvatarBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOmniAvatarBridgeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Omni|AvatarBridge")
	FGameplayTagContainer GetContextTags() const;

	UFUNCTION(BlueprintPure, Category = "Omni|AvatarBridge")
	float GetBaseSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Omni|AvatarBridge")
	float GetEffectiveSpeed() const;

private:
	void ResolveHost();
	void ResolveRegistryAndSystems();
	void ResolveConfig();
	void RefreshContextTags();
	void ApplyMovementBackend();
	void EnsureHud();

private:
	UPROPERTY(EditAnywhere, Category = "Omni|AvatarBridge")
	bool bAutoEnsureHud = true;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> HostPawn;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniMovementSystem> MovementSystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniUIBridgeSubsystem> UIBridgeSubsystem;

	UPROPERTY(Transient)
	FGameplayTagContainer ContextTags;

	UPROPERTY(Transient)
	FGameplayTag SprintRequestedTag;

	UPROPERTY(Transient)
	FGameplayTag CameraModeTpsTag;

	UPROPERTY(Transient)
	float BaseSpeed = 0.0f;

	UPROPERTY(Transient)
	float EffectiveSpeed = 0.0f;

	UPROPERTY(Transient)
	float SprintMultiplier = 1.6f;

	UPROPERTY(Transient)
	bool bHasCapturedBaseSpeed = false;

	bool bLoggedInvalidHost = false;
	bool bLoggedMissingCmc = false;
	bool bLoggedMissingRegistry = false;
	bool bLoggedMissingMovementSystem = false;
	bool bLoggedHudFallback = false;
	bool bLoggedHudReady = false;
};

