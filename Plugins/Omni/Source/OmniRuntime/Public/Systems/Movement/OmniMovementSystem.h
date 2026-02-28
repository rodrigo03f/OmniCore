#pragma once

#include "CoreMinimal.h"
#include "Systems/Movement/OmniMovementData.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniMovementSystem.generated.h"

class UOmniManifest;
class UOmniDebugSubsystem;
class UOmniSystemRegistrySubsystem;
class UOmniClockSubsystem;

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
	virtual void HandleEvent_Implementation(const FOmniEventMessage& Event) override;

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
	bool TryLoadSettingsFromManifest(const UOmniManifest* Manifest, bool bAllowDevDefaults, FString& OutError);
	bool BuildDevFallbackSettings();
	void StartSprinting();
	void StopSprinting(FName Reason);
	void PublishTelemetry() const;
	double GetNowSeconds() const;
	bool QueryStatusIsExhausted() const;
	void DispatchStatusSprinting(bool bSprinting) const;
	bool QueryCanStartSprint(FString* OutReason = nullptr) const;
	bool DispatchStartSprint() const;
	bool DispatchStopSprint(FName Reason) const;

private:
	UPROPERTY(Transient)
	FOmniMovementSettings RuntimeSettings;

	UPROPERTY(Transient)
	bool bSprintRequested = false;

	UPROPERTY(Transient)
	bool bIsSprinting = false;

	UPROPERTY(Transient)
	float NextStartAttemptWorldTime = 0.0f;

	UPROPERTY(Transient)
	float AutoSprintRemainingSeconds = 0.0f;

	UPROPERTY(Transient)
	bool bObservedSprintStartedEvent = false;

	UPROPERTY(Transient)
	bool bObservedSprintEndedEvent = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniClockSubsystem> ClockSubsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;
};
