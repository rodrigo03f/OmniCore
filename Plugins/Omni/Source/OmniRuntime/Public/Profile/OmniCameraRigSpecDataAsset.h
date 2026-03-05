#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Camera/OmniCameraData.h"
#include "OmniCameraRigSpecDataAsset.generated.h"

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniCameraRigSpecDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Camera")
	FOmniCameraRigSpec RigSpec;
};
