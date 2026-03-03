#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "OmniModifiersData.generated.h"

UENUM(BlueprintType)
enum class EOmniModifierOperation : uint8
{
	Add UMETA(DisplayName = "Add"),
	Mul UMETA(DisplayName = "Mul")
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniModifierSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Modifiers")
	FGameplayTag ModifierTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Modifiers")
	FGameplayTag TargetTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Modifiers")
	EOmniModifierOperation Operation = EOmniModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Modifiers")
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Modifiers")
	FName SourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Modifiers", meta = (ClampMin = "0.0"))
	float DurationSeconds = 0.0f;

	bool IsValid(FString& OutError) const
	{
		OutError.Reset();

		if (!ModifierTag.IsValid())
		{
			OutError = TEXT("ModifierTag must be valid.");
			return false;
		}
		if (!TargetTag.IsValid())
		{
			OutError = TEXT("TargetTag must be valid.");
			return false;
		}
		if (SourceId == NAME_None)
		{
			OutError = TEXT("SourceId must not be None.");
			return false;
		}
		if (DurationSeconds < 0.0f)
		{
			OutError = TEXT("DurationSeconds must be >= 0.");
			return false;
		}
		return true;
	}
};
