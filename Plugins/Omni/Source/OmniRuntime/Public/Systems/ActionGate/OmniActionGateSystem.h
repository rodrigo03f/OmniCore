#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/OmniRuntimeSystem.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "OmniActionGateSystem.generated.h"

class UOmniManifest;
class UOmniStatusSystem;
class UOmniSystemRegistrySubsystem;
class UOmniDebugSubsystem;

UCLASS()
class OMNIRUNTIME_API UOmniActionGateSystem : public UOmniRuntimeSystem
{
	GENERATED_BODY()

public:
	virtual FName GetSystemId_Implementation() const override;
	virtual TArray<FName> GetDependencies_Implementation() const override;
	virtual void InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest) override;
	virtual void ShutdownSystem_Implementation() override;
	virtual bool IsTickEnabled_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	bool TryStartAction(FName ActionId, FOmniActionGateDecision& OutDecision);

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	bool StopAction(FName ActionId, FName Reason = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	bool IsActionActive(FName ActionId) const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	TArray<FName> GetActiveActions() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	FGameplayTagContainer GetActiveLocks() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	TArray<FName> GetKnownActionIds() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	FOmniActionGateDecision GetLastDecision() const;

private:
	void BuildDefaultDefinitionsIfNeeded();
	void RebuildDefinitionMap();
	FGameplayTagContainer BuildCurrentBlockingContext() const;
	UOmniStatusSystem* ResolveStatusSystem() const;
	void AddActionLocks(const FOmniActionDefinition& Definition);
	void RemoveActionLocks(const FOmniActionDefinition& Definition);
	void PublishTelemetry();
	void PublishDecision(const FOmniActionGateDecision& Decision, bool bEmitLogEntry);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|ActionGate")
	TArray<FOmniActionDefinition> DefaultDefinitions;

	UPROPERTY(Transient)
	TMap<FName, FOmniActionDefinition> DefinitionsById;

	UPROPERTY(Transient)
	TSet<FName> ActiveActions;

	UPROPERTY(Transient)
	TMap<FGameplayTag, int32> ActiveLockRefCounts;

	UPROPERTY(Transient)
	FOmniActionGateDecision LastDecision;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;

	UPROPERTY(Transient)
	bool bInitialized = false;
};
