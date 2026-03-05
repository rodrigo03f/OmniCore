#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "OmniAnimInstanceBase.generated.h"

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniAnimBridgeFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsSprinting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsExhausted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsCrouching = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsInAir = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	float VerticalSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	FGameplayTag ActiveAnimSetTag;
};

// Purpose:
// - Expor variaveis canonicas de locomocao/status para AnimBlueprints Omni.
// - Concentrar contrato minimo de animacao (bridge-only, sem acesso direto a systems).
// Inputs:
// - Valores preenchidos por UOmniAnimBridgeComponent.
// Outputs:
// - Variaveis BlueprintReadWrite consumidas por AnimGraph/StateMachine.
// Determinism:
// - Apenas espelha estado recebido do bridge (sem logica temporal propria).
// Failure modes:
// - Tag de anim set ausente em projeto cai para tag invalida (sem crash).
UCLASS(Blueprintable)
class OMNIRUNTIME_API UOmniAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	UOmniAnimInstanceBase();

	virtual void NativeInitializeAnimation() override;

	// Contrato canônico de escrita para o bridge:
	// todo update de estado de animacao deve passar por este frame.
	UFUNCTION(BlueprintCallable, Category = "Omni|AnimBridge")
	void ApplyBridgeFrame(const FOmniAnimBridgeFrame& BridgeFrame);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsSprinting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsExhausted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsCrouching = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	bool bIsInAir = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	float VerticalSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|AnimBridge")
	FGameplayTag ActiveAnimSetTag;
};
