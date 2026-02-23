#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OmniRuntimeSystem.generated.h"

class UOmniManifest;

UCLASS(Abstract, Blueprintable)
class OMNICORE_API UOmniRuntimeSystem : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System")
	FName GetSystemId() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System")
	TArray<FName> GetDependencies() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System")
	void InitializeSystem(UObject* WorldContextObject, const UOmniManifest* Manifest);

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System")
	void ShutdownSystem();

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System")
	bool IsTickEnabled() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System")
	void TickSystem(float DeltaTime);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|System")
	FName SystemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|System")
	TArray<FName> Dependencies;
};

