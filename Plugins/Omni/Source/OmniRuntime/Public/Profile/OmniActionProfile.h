#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "OmniActionProfile.generated.h"

class UOmniActionLibrary;

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniActionProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	TSoftObjectPtr<UOmniActionLibrary> ActionLibrary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|ActionGate")
	TArray<FOmniActionDefinition> OverrideDefinitions;

	void ResolveDefinitions(TArray<FOmniActionDefinition>& OutDefinitions) const;
};

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniDevActionProfile : public UOmniActionProfile
{
	GENERATED_BODY()

public:
	UOmniDevActionProfile();
};
