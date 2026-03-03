#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "OmniAnimBridgeComponent.generated.h"

class APawn;
class UCharacterMovementComponent;
class UOmniAnimInstanceBase;
class UOmniMovementSystem;
class UOmniStatusSystem;
class UOmniSystemRegistrySubsystem;
class USkeletalMeshComponent;

// Purpose:
// - Bridge unico entre runtime Omni e AnimInstance (sem consulta direta no AnimBP).
// - Resolver host mesh/animinstance e aplicar variaveis canonicas por tick.
// Inputs:
// - Estado de movement/status no registry Omni.
// - Fallback de locomocao via CMC (crouch/inair/vertical speed).
// Outputs:
// - Variaveis em UOmniAnimInstanceBase.
// Determinism:
// - Atualizacao derivada de snapshots/tags e velocidade atual do pawn.
// Failure modes:
// - Host/mesh/animinstance invalidos entram em fallback com log unico.
UCLASS(ClassGroup = (Omni), meta = (BlueprintSpawnableComponent))
class OMNIRUNTIME_API UOmniAnimBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOmniAnimBridgeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Omni|AnimBridge")
	bool IsBridgeReady() const;

	UFUNCTION(BlueprintPure, Category = "Omni|AnimBridge")
	bool IsOmniAnimInstanceResolved() const;

	UFUNCTION(BlueprintPure, Category = "Omni|AnimBridge")
	float GetDebugSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Omni|AnimBridge")
	bool GetDebugIsSprinting() const;

	UFUNCTION(BlueprintPure, Category = "Omni|AnimBridge")
	bool GetDebugIsExhausted() const;

private:
	void ResolveHost();
	void ResolveRegistryAndSystems();
	void ResolveAnimationTargets();
	void ApplyOmniState();
	void ApplyBodyFallbackSlots();

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> HostPawn;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniAnimInstanceBase> OmniAnimInstance;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniMovementSystem> MovementSystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniStatusSystem> StatusSystem;

	UPROPERTY(Transient)
	FGameplayTag DefaultAnimSetTag;

	UPROPERTY(Transient)
	float DebugSpeed = 0.0f;

	UPROPERTY(Transient)
	bool bDebugIsSprinting = false;

	UPROPERTY(Transient)
	bool bDebugIsExhausted = false;

	bool bLoggedInvalidHost = false;
	bool bLoggedMissingRegistry = false;
	bool bLoggedMissingMesh = false;
	bool bLoggedInvalidAnimInstance = false;
	bool bLoggedReady = false;
};
