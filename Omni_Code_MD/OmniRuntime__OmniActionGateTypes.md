# OmniRuntime / OmniActionGateTypes

## Arquivos agrupados
- Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateTypes.h

## Header: Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateTypes.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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

USTRUCT(BlueprintType)
struct FOmniActionGateDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	FName ActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	bool bAllowed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	FString Reason;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	EOmniActionPolicy Policy = EOmniActionPolicy::DenyIfActive;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|ActionGate")
	TArray<FName> CanceledActions;
};

```

