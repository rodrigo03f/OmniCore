#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "OmniGateTypes.generated.h"

UENUM(BlueprintType)
enum class EOmniActionGateLockScope : uint8
{
	Global UMETA(DisplayName = "Global"),
	Action UMETA(DisplayName = "Action")
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniGateDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Gate")
	bool bAllowed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Gate")
	FGameplayTag ReasonTag;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Gate")
	FString ReasonText;
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniGateLock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Gate")
	FGameplayTag LockTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Gate")
	FName SourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Gate")
	EOmniActionGateLockScope Scope = EOmniActionGateLockScope::Global;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Gate", meta = (EditCondition = "Scope == EOmniActionGateLockScope::Action"))
	FGameplayTag ActionTag;

	bool IsValid() const
	{
		if (!LockTag.IsValid() || SourceId == NAME_None)
		{
			return false;
		}

		if (Scope == EOmniActionGateLockScope::Action)
		{
			return ActionTag.IsValid();
		}

		return true;
	}
};
