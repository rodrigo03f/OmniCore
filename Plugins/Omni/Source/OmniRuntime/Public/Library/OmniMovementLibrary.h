#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Movement/OmniMovementData.h"
#include "OmniMovementLibrary.generated.h"

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniMovementLibrary : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Movement")
	FOmniMovementSettings Settings;
};

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniDevMovementLibrary : public UOmniMovementLibrary
{
	GENERATED_BODY()

public:
	UOmniDevMovementLibrary();
};
