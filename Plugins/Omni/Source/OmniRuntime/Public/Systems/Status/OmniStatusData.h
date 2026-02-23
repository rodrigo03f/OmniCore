#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "OmniStatusData.generated.h"

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniStatusSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "1.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.1"))
	float SprintDrainPerSecond = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.1"))
	float RegenPerSecond = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float RegenDelaySeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float ExhaustRecoverThreshold = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status")
	FGameplayTag ExhaustedTag;

	bool IsValid(FString& OutError) const
	{
		OutError.Reset();

		if (MaxStamina <= KINDA_SMALL_NUMBER)
		{
			OutError = TEXT("MaxStamina must be > 0.");
			return false;
		}
		if (SprintDrainPerSecond <= KINDA_SMALL_NUMBER)
		{
			OutError = TEXT("SprintDrainPerSecond must be > 0.");
			return false;
		}
		if (RegenPerSecond <= KINDA_SMALL_NUMBER)
		{
			OutError = TEXT("RegenPerSecond must be > 0.");
			return false;
		}
		if (RegenDelaySeconds < 0.0f)
		{
			OutError = TEXT("RegenDelaySeconds must be >= 0.");
			return false;
		}
		if (ExhaustRecoverThreshold < 0.0f || ExhaustRecoverThreshold > MaxStamina)
		{
			OutError = TEXT("ExhaustRecoverThreshold must be in range [0, MaxStamina].");
			return false;
		}

		return true;
	}
};
