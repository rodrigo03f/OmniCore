#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/OmniRuntimeSystem.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "OmniActionGateSystem.generated.h"

class UOmniManifest;
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
	virtual bool HandleCommand_Implementation(const FOmniCommandMessage& Command) override;
	virtual bool HandleQuery_Implementation(FOmniQueryMessage& Query) override;
	virtual void HandleEvent_Implementation(const FOmniEventMessage& Event) override;

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
	bool TryLoadDefinitionsFromManifest(const UOmniManifest* Manifest, FString& OutError);
	void RebuildDefinitionMap();
	void BroadcastActionLifecycleEvent(FName EventName, FName ActionId, const FString& Reason = FString(), FName EndReason = NAME_None);
	FGameplayTagContainer BuildCurrentBlockingContext() const;
	bool EvaluateStartAction(FName ActionId, FOmniActionGateDecision& OutDecision, bool bApplyChanges);
	static bool TryParseActionId(const FOmniQueryMessage& Query, FName& OutActionId);
	void AddActionLocks(const FOmniActionDefinition& Definition);
	void RemoveActionLocks(const FOmniActionDefinition& Definition);
	void PublishTelemetry();
	void PublishDecision(const FOmniActionGateDecision& Decision, bool bEmitLogEntry);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|ActionGate")
	TArray<FOmniActionDefinition> DefaultDefinitions;

	UPROPERTY(Transient)
	FString ResolvedProfileName;

	UPROPERTY(Transient)
	FString ResolvedProfileAssetPath;

	UPROPERTY(Transient)
	FString ResolvedLibraryAssetPath;

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
