#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Systems/OmniSystemMessaging.h"
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

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System|Messaging")
	bool HandleCommand(const FOmniCommandMessage& Command);

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System|Messaging")
	bool HandleQuery(UPARAM(ref) FOmniQueryMessage& Query);

	UFUNCTION(BlueprintNativeEvent, Category = "Omni|System|Messaging")
	void HandleEvent(const FOmniEventMessage& Event);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|System")
	FName SystemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|System")
	TArray<FName> Dependencies;
};
