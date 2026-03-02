#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/OmniRuntimeSystem.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "Systems/ActionGate/OmniGateTypes.h"
#include "OmniActionGateSystem.generated.h"

class UOmniManifest;
class UOmniSystemRegistrySubsystem;
class UOmniDebugSubsystem;

// Purpose:
// - Aplicar regras de acao (allow/deny) com base em definicoes data-driven.
// - Controlar locks de gameplay tags por acao ativa.
// - Publicar eventos de ciclo de vida de acao (start/end/deny).
// Inputs:
// - Manifest/profile/library com definicoes de acao.
// - Commands/Queries/Events recebidos via registry.
// - Estado atual de acoes ativas e locks.
// Outputs:
// - Decisao da acao (FOmniGateDecision).
// - Conjunto de acoes ativas e tags de lock ativas.
// - Eventos para outros systems interessados.
// Determinism:
// - Listas publicas retornadas de forma ordenada (acoes e locks).
// - Emissao de erros de validacao com ordem estavel por ActionId.
// Failure modes:
// - Dados invalidos no profile/library podem causar fail-fast (modo estrito).
// - ActionId inexistente ou desabilitado retorna deny explicito.
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
	FOmniGateDecision Can(FGameplayTag ActionTag, const FGameplayTagContainer& ContextTags) const;

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	FOmniGateDecision StartAction(FName ActionId);

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	bool AddLock(const FOmniGateLock& Lock);

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	bool RemoveLock(const FOmniGateLock& Lock);

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	int32 GetActiveLockCount() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	bool StopAction(FName ActionId, FName Reason = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	TArray<FOmniGateLock> GetActiveLockSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	bool IsActionActive(FName ActionId) const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	TArray<FName> GetActiveActions() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	FGameplayTagContainer GetActiveLocks() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	TArray<FName> GetKnownActionIds() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	FOmniGateDecision GetLastDecision() const;

private:
	bool TryLoadDefinitionsFromManifest(const UOmniManifest* Manifest, FString& OutError);
	void RebuildDefinitionMap();
	void BroadcastActionLifecycleEvent(FName EventName, FName ActionId, const FString& Reason = FString(), FName EndReason = NAME_None);
	FGameplayTagContainer BuildCurrentBlockingContext() const;
	FOmniGateDecision EvaluateStartAction(FName ActionId, bool bApplyChanges);
	FGameplayTag ResolveActionTag(FName ActionId) const;
	void RemoveAllActionLocks(FName ActionId);
	TArray<FOmniGateLock> BuildSortedLocks() const;
	static bool IsScopeLess(EOmniActionGateLockScope Left, EOmniActionGateLockScope Right);
	static bool TryParseActionId(const FOmniQueryMessage& Query, FName& OutActionId);
	void AddActionLocks(const FOmniActionDefinition& Definition);
	void PublishTelemetry();
	void PublishDecision(const FOmniGateDecision& Decision, FName ActionId, bool bEmitLogEntry);

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
	TArray<FOmniGateLock> ActiveLocks;

	TMap<FName, TArray<FOmniGateLock>> LocksByAction;

	UPROPERTY(Transient)
	FOmniGateDecision LastDecision;

	UPROPERTY(Transient)
	FGameplayTag AllowReasonTag;

	UPROPERTY(Transient)
	FGameplayTag DenyReasonTag;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;

	UPROPERTY(Transient)
	bool bInitialized = false;
};
