#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Status/OmniStatusData.h"
#include "OmniStatusLibrary.generated.h"

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniStatusLibrary : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status")
	FOmniStatusSettings Settings;
};

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniDevStatusLibrary : public UOmniStatusLibrary
{
	GENERATED_BODY()

public:
	UOmniDevStatusLibrary();
};
