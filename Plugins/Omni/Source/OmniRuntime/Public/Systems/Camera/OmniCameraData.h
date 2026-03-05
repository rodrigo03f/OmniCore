#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "OmniCameraData.generated.h"

UENUM(BlueprintType)
enum class EOmniCameraMode : uint8
{
	FPS UMETA(DisplayName = "FPS"),
	TPS UMETA(DisplayName = "TPS"),
	TopDown UMETA(DisplayName = "TopDown"),
	Ortho2D UMETA(DisplayName = "Ortho2D")
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniCameraRigSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera")
	EOmniCameraMode CameraMode = EOmniCameraMode::TPS;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera", meta = (ClampMin = "10.0", ClampMax = "170.0"))
	float FOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera", meta = (ClampMin = "1.0"))
	float OrthoWidth = 2048.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera")
	FVector Offset = FVector(0.0f, 0.0f, 60.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera", meta = (ClampMin = "0.0"))
	float ArmLength = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera")
	bool bUseLag = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera", meta = (ClampMin = "0.0"))
	float LagSpeed = 12.0f;

	bool IsValid(FString& OutError) const
	{
		OutError.Reset();

		if (CameraMode == EOmniCameraMode::Ortho2D)
		{
			if (OrthoWidth <= KINDA_SMALL_NUMBER)
			{
				OutError = TEXT("OrthoWidth must be > 0 for Ortho2D mode.");
				return false;
			}
		}
		else if (FOV <= 10.0f || FOV >= 170.0f)
		{
			OutError = TEXT("FOV must be in range (10, 170) for perspective modes.");
			return false;
		}

		if (ArmLength < 0.0f)
		{
			OutError = TEXT("ArmLength must be >= 0.");
			return false;
		}
		if (bUseLag && LagSpeed < 0.0f)
		{
			OutError = TEXT("LagSpeed must be >= 0 when lag is enabled.");
			return false;
		}

		return true;
	}
};

USTRUCT(BlueprintType)
struct OMNIRUNTIME_API FOmniCameraSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Camera")
	FGameplayTag ActiveModeTag;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Camera")
	FName ActiveRigName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Camera")
	float CurrentFOV = 90.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Camera")
	float CurrentOrthoWidth = 2048.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Camera")
	float CurrentArmLength = 300.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Camera")
	FVector CurrentOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Camera")
	bool bUsingFallback = false;
};
