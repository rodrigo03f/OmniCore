#pragma once

#include "CoreMinimal.h"
#include "OmniMovementData.generated.h"

UENUM(BlueprintType)
enum class EOmniMovementMode : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Sprint UMETA(DisplayName = "Sprint")
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniMovementSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement")
	FName SprintActionId = TEXT("Movement.Sprint");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement", meta = (ClampMin = "0.05"))
	float FailedRetryIntervalSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement", meta = (ClampMin = "1.0"))
	float SprintMultiplier = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement")
	bool bUseKeyboardShiftAsSprintRequest = true;

	bool IsValid(FString& OutError) const
	{
		OutError.Reset();

		if (SprintActionId == NAME_None)
		{
			OutError = TEXT("SprintActionId must not be None.");
			return false;
		}
		if (FailedRetryIntervalSeconds <= KINDA_SMALL_NUMBER)
		{
			OutError = TEXT("FailedRetryIntervalSeconds must be > 0.");
			return false;
		}
		if (SprintMultiplier < 1.0f)
		{
			OutError = TEXT("SprintMultiplier must be >= 1.");
			return false;
		}

		return true;
	}
};
