#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OmniMotionMatchingSubsystem.generated.h"

struct FOmniMMState
{
	bool bEnabled = false;
	FGameplayTag ActiveAnimSetTag;
};

UCLASS()
class OMNIEXPERIMENTAL_API UOmniMotionMatchingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	FString BuildStatusLine() const;

private:
	void GatherRuntimeInputs(float& OutSpeed, bool& OutIsSprinting, bool& OutIsExhausted) const;
	FString EvaluatePlaceholderDecision(float Speed, bool bIsSprinting, bool bIsExhausted) const;
	FString GetActiveAnimSetTagString() const;

private:
	FOmniMMState RuntimeState;
};
