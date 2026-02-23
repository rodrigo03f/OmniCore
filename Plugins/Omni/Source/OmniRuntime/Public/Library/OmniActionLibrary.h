#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "OmniActionLibrary.generated.h"

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniActionLibrary : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	TArray<FOmniActionDefinition> Definitions;
};

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniDevActionLibrary : public UOmniActionLibrary
{
	GENERATED_BODY()

public:
	UOmniDevActionLibrary();
};
