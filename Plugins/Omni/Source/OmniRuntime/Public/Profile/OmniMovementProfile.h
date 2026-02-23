#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Movement/OmniMovementData.h"
#include "UObject/SoftObjectPtr.h"
#include "OmniMovementProfile.generated.h"

class UOmniMovementLibrary;

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniMovementProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement")
	TSoftObjectPtr<UOmniMovementLibrary> MovementLibrary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement")
	bool bUseOverrides = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement", meta = (EditCondition = "bUseOverrides"))
	FOmniMovementSettings OverrideSettings;

	bool ResolveSettings(FOmniMovementSettings& OutSettings) const;
};

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniDevMovementProfile : public UOmniMovementProfile
{
	GENERATED_BODY()

public:
	UOmniDevMovementProfile();
};
