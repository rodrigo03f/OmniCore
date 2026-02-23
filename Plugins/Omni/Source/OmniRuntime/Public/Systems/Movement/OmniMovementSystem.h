#pragma once

#include "CoreMinimal.h"
#include "Systems/OmniRuntimeSystem.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "OmniMovementSystem.generated.h"

class UOmniManifest;
class UOmniDebugSubsystem;
class UOmniSystemRegistrySubsystem;
class UOmniActionGateSystem;
class UOmniStatusSystem;

UCLASS()
class OMNIRUNTIME_API UOmniMovementSystem : public UOmniRuntimeSystem
{
	GENERATED_BODY()

public:
	virtual FName GetSystemId_Implementation() const override;
	virtual TArray<FName> GetDependencies_Implementation() const override;
	virtual void InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest) override;
	virtual void ShutdownSystem_Implementation() override;
	virtual bool IsTickEnabled_Implementation() const override;
	virtual void TickSystem_Implementation(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Omni|Movement")
	void SetSprintRequested(bool bRequested);

	UFUNCTION(BlueprintCallable, Category = "Omni|Movement")
	void ToggleSprintRequested();

	UFUNCTION(BlueprintCallable, Category = "Omni|Movement")
	void StartAutoSprint(float DurationSeconds);

	UFUNCTION(BlueprintPure, Category = "Omni|Movement")
	bool IsSprintRequested() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Movement")
	bool IsSprinting() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Movement")
	float GetAutoSprintRemainingSeconds() const;

private:
	bool TryResolveDependencies();
	void StartSprinting();
	void StopSprinting(FName Reason);
	void PublishTelemetry() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|Movement")
	FName SprintActionId = TEXT("Movement.Sprint");

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Movement", meta = (ClampMin = "0.05"))
	float FailedRetryIntervalSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Movement")
	bool bUseKeyboardShiftAsSprintRequest = true;

	UPROPERTY(Transient)
	bool bSprintRequested = false;

	UPROPERTY(Transient)
	bool bIsSprinting = false;

	UPROPERTY(Transient)
	float NextStartAttemptWorldTime = 0.0f;

	UPROPERTY(Transient)
	float AutoSprintRemainingSeconds = 0.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniActionGateSystem> ActionGateSystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniStatusSystem> StatusSystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;
};
