#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OmniClockSubsystem.generated.h"

UCLASS()
class OMNIRUNTIME_API UOmniClockSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	UFUNCTION(BlueprintPure, Category = "Omni|Clock")
	double GetSimTime() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Clock")
	int64 GetTickIndex() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Clock")
	void ResetClock();

private:
	UPROPERTY(EditAnywhere, Category = "Omni|Clock")
	bool bUseWorldTimeProvider = true;

	UPROPERTY(Transient)
	double SimTimeSeconds = 0.0;

	UPROPERTY(Transient)
	int64 TickIndex = 0;
};
