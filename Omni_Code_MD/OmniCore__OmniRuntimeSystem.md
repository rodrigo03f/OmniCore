# OmniCore / OmniRuntimeSystem

## Arquivos agrupados
- Plugins/Omni/Source/OmniCore/Private/Systems/OmniRuntimeSystem.cpp
- Plugins/Omni/Source/OmniCore/Public/Systems/OmniRuntimeSystem.h

## Header: Plugins/Omni/Source/OmniCore/Public/Systems/OmniRuntimeSystem.h
```cpp
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


```

## Source: Plugins/Omni/Source/OmniCore/Private/Systems/OmniRuntimeSystem.cpp
```cpp
#include "Systems/OmniRuntimeSystem.h"

FName UOmniRuntimeSystem::GetSystemId_Implementation() const
{
	if (SystemId != NAME_None)
	{
		return SystemId;
	}

	return GetClass() ? GetClass()->GetFName() : NAME_None;
}

TArray<FName> UOmniRuntimeSystem::GetDependencies_Implementation() const
{
	return Dependencies;
}

void UOmniRuntimeSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	(void)WorldContextObject;
	(void)Manifest;
}

void UOmniRuntimeSystem::ShutdownSystem_Implementation()
{
}

bool UOmniRuntimeSystem::IsTickEnabled_Implementation() const
{
	return false;
}

void UOmniRuntimeSystem::TickSystem_Implementation(float DeltaTime)
{
	(void)DeltaTime;
}


```

