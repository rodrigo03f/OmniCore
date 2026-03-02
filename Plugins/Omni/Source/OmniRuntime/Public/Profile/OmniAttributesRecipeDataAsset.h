#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Attributes/OmniAttributeTypes.h"
#include "OmniAttributesRecipeDataAsset.generated.h"

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniAttributesRecipeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Attributes")
	TArray<FOmniAttributeRecipeEntry> Attributes;
};
