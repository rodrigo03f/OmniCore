#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniStatusSystem.generated.h"

class UOmniManifest;
class UOmniDebugSubsystem;
class UOmniSystemRegistrySubsystem;

UCLASS()
class OMNIRUNTIME_API UOmniStatusSystem : public UOmniRuntimeSystem
{
	GENERATED_BODY()

public:
	virtual FName GetSystemId_Implementation() const override;
	virtual TArray<FName> GetDependencies_Implementation() const override;
	virtual void InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest) override;
	virtual void ShutdownSystem_Implementation() override;
	virtual bool IsTickEnabled_Implementation() const override;
	virtual void TickSystem_Implementation(float DeltaTime) override;
	virtual bool HandleCommand_Implementation(const FOmniCommandMessage& Command) override;
	virtual bool HandleQuery_Implementation(FOmniQueryMessage& Query) override;
	virtual void HandleEvent_Implementation(const FOmniEventMessage& Event) override;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	float GetStaminaNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	bool IsExhausted() const;

	const FGameplayTagContainer& GetStateTags() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	void SetSprinting(bool bInSprinting);

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	void ConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	void AddStamina(float Amount);

private:
	void UpdateStateTags();
	void PublishTelemetry();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "1.0"))
	float MaxStaminaValue = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.1"))
	float SprintDrainPerSecond = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.1"))
	float RegenPerSecond = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.0"))
	float RegenDelaySeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.0"))
	float ExhaustRecoverThreshold = 22.0f;

	UPROPERTY(Transient)
	float CurrentStamina = 100.0f;

	UPROPERTY(Transient)
	bool bSprinting = false;

	UPROPERTY(Transient)
	bool bExhausted = false;

	UPROPERTY(Transient)
	float TimeSinceLastConsumption = 0.0f;

	UPROPERTY(Transient)
	FGameplayTagContainer StateTags;

	UPROPERTY(Transient)
	FGameplayTag ExhaustedTag;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;
};
