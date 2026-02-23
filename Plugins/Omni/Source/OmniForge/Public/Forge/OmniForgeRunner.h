#pragma once

#include "CoreMinimal.h"
#include "Forge/OmniForgeTypes.h"
#include "UObject/Object.h"
#include "OmniForgeRunner.generated.h"

UCLASS()
class OMNIFORGE_API UOmniForgeRunner : public UObject
{
	GENERATED_BODY()

public:
	static FOmniForgeResult Run(const FOmniForgeInput& Input);
};
