#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/ActionGate/OmniGateTypes.h"
#include "OmniActionGateTypes.generated.h"

UENUM(BlueprintType)
enum class EOmniActionPolicy : uint8
{
	DenyIfActive UMETA(DisplayName = "Deny If Active"),
	SucceedIfActive UMETA(DisplayName = "Succeed If Active"),
	RestartIfActive UMETA(DisplayName = "Restart If Active")
};

USTRUCT(BlueprintType)
struct FOmniActionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	FGameplayTagContainer BlockedBy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	TArray<FName> Cancels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	FGameplayTagContainer AppliesLocks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	EOmniActionPolicy Policy = EOmniActionPolicy::DenyIfActive;
};

USTRUCT(BlueprintType, meta = (Deprecated, DeprecationMessage = "Use FOmniGateDecision from OmniGateTypes.h"))
struct FOmniActionGateDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	FName ActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	FOmniGateDecision Decision;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	EOmniActionPolicy Policy = EOmniActionPolicy::DenyIfActive;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	TArray<FName> CanceledActions;

	bool IsAllowed() const
	{
		return Decision.bAllowed;
	}
};
