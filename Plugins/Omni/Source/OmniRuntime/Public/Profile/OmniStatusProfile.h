#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Status/OmniStatusData.h"
#include "UObject/SoftObjectPtr.h"
#include "OmniStatusProfile.generated.h"

class UOmniStatusLibrary;

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniStatusProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status")
	TSoftObjectPtr<UOmniStatusLibrary> StatusLibrary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status")
	bool bUseOverrides = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Status", meta = (EditCondition = "bUseOverrides"))
	FOmniStatusSettings OverrideSettings;

	bool ResolveSettings(FOmniStatusSettings& OutSettings) const;
};

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniDevStatusProfile : public UOmniStatusProfile
{
	GENERATED_BODY()

public:
	UOmniDevStatusProfile();
};
