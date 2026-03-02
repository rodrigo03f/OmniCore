#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "OmniAttributeTypes.generated.h"

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniAttributeValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes")
	float Current = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes")
	float Max = 0.0f;
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniStaminaRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes|Stamina", meta = (ClampMin = "0.0"))
	float DrainPerSecond = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes|Stamina", meta = (ClampMin = "0.0"))
	float RegenPerSecond = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes|Stamina", meta = (ClampMin = "0.0"))
	float RegenDelaySeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes|Stamina", meta = (ClampMin = "0.0"))
	float ExhaustedThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes|Stamina", meta = (ClampMin = "0.0"))
	float RecoverThreshold = 22.0f;

	bool IsValid(float StaminaMax, FString& OutError) const
	{
		OutError.Reset();
		if (DrainPerSecond < 0.0f)
		{
			OutError = TEXT("DrainPerSecond must be >= 0.");
			return false;
		}
		if (RegenPerSecond < 0.0f)
		{
			OutError = TEXT("RegenPerSecond must be >= 0.");
			return false;
		}
		if (RegenDelaySeconds < 0.0f)
		{
			OutError = TEXT("RegenDelaySeconds must be >= 0.");
			return false;
		}
		if (ExhaustedThreshold < 0.0f || ExhaustedThreshold > StaminaMax)
		{
			OutError = TEXT("ExhaustedThreshold must be in [0, StaminaMax].");
			return false;
		}
		if (RecoverThreshold < 0.0f || RecoverThreshold > StaminaMax)
		{
			OutError = TEXT("RecoverThreshold must be in [0, StaminaMax].");
			return false;
		}
		if (RecoverThreshold < ExhaustedThreshold)
		{
			OutError = TEXT("RecoverThreshold must be >= ExhaustedThreshold.");
			return false;
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniAttributeRecipeEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes")
	FGameplayTag AttrTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes", meta = (ClampMin = "0.0"))
	float MaxValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes", meta = (ClampMin = "0.0"))
	float StartValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes")
	bool bUseStaminaRules = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Attributes", meta = (EditCondition = "bUseStaminaRules"))
	FOmniStaminaRules StaminaRules;

	bool IsValid(FString& OutError) const
	{
		OutError.Reset();
		if (!AttrTag.IsValid())
		{
			OutError = TEXT("AttrTag must be valid.");
			return false;
		}
		if (MaxValue <= KINDA_SMALL_NUMBER)
		{
			OutError = TEXT("MaxValue must be > 0.");
			return false;
		}
		if (StartValue < 0.0f || StartValue > MaxValue)
		{
			OutError = TEXT("StartValue must be in [0, MaxValue].");
			return false;
		}
		if (bUseStaminaRules && !StaminaRules.IsValid(MaxValue, OutError))
		{
			return false;
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniAttributeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Attributes")
	FOmniAttributeValue HP;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Attributes")
	FOmniAttributeValue Stamina;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Attributes")
	bool bExhausted = false;
};
