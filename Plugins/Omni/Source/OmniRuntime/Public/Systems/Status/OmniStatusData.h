#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "OmniStatusData.generated.h"

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniStatusSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float SprintDrainPerSecond = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float RegenPerSecond = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float RegenDelaySeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float ExhaustedThreshold = 0.0f;

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
		if (ExhaustedThreshold < 0.0f || ExhaustedThreshold > MaxStamina)
		{
			OutError = TEXT("ExhaustedThreshold must be in range [0, MaxStamina].");
			return false;
		}
		if (ExhaustRecoverThreshold < 0.0f || ExhaustRecoverThreshold > MaxStamina)
		{
			OutError = TEXT("ExhaustRecoverThreshold must be in range [0, MaxStamina].");
			return false;
		}
		if (ExhaustRecoverThreshold < ExhaustedThreshold)
		{
			OutError = TEXT("ExhaustRecoverThreshold must be >= ExhaustedThreshold.");
			return false;
		}

		return true;
	}
};

UENUM(BlueprintType)
enum class EOmniStatusRefreshPolicy : uint8
{
	RefreshDuration UMETA(DisplayName = "RefreshDuration"),
	ExtendDuration UMETA(DisplayName = "ExtendDuration"),
	IgnoreDuration UMETA(DisplayName = "IgnoreDuration")
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniStatusRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status")
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float DefaultDurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status")
	bool bStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "1"))
	int32 MaxStacks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status")
	EOmniStatusRefreshPolicy RefreshPolicy = EOmniStatusRefreshPolicy::RefreshDuration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (ClampMin = "0.0"))
	float TickIntervalSeconds = 0.0f;

	bool IsValid(FString& OutError) const
	{
		OutError.Reset();

		if (!StatusTag.IsValid())
		{
			OutError = TEXT("StatusTag must be valid.");
			return false;
		}

		if (DefaultDurationSeconds < 0.0f)
		{
			OutError = TEXT("DefaultDurationSeconds must be >= 0.");
			return false;
		}

		if (MaxStacks <= 0)
		{
			OutError = TEXT("MaxStacks must be >= 1.");
			return false;
		}

		if (!bStackable && MaxStacks != 1)
		{
			OutError = TEXT("Non-stackable statuses must use MaxStacks = 1.");
			return false;
		}

		if (TickIntervalSeconds < 0.0f)
		{
			OutError = TEXT("TickIntervalSeconds must be >= 0.");
			return false;
		}

		return true;
	}
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniStatusEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Status")
	FGameplayTag StatusTag;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Status")
	FName SourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Status")
	int32 Stacks = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Status")
	float RemainingTimeSeconds = 0.0f;
};
