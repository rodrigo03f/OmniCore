#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/Status/OmniStatusData.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniStatusSystem.generated.h"

class UOmniManifest;
class UOmniDebugSubsystem;
class UOmniSystemRegistrySubsystem;
class UOmniClockSubsystem;
class UOmniAttributesSystem;

// Purpose:
// - Orquestrar status temporarios (duracao, stacks, tick e tags de status).
// - Publicar/expirar efeitos de status e sincronizar bridges (ex.: Modifiers).
// Inputs:
// - Commands/queries de status.
// - Tempo de simulacao via OmniClock para expiracao deterministica.
// Outputs:
// - Snapshot de status ativos.
// - Gameplay tags agregadas (status + attributes) para compatibilidade temporaria.
// - Telemetria de debug do slice de status.
// Determinism:
// - Duracao/expiracao de status depende apenas de recipe + OmniClock.
// - Ordenacao estavel de effects ativos e snapshot.
// Failure modes:
// - Payload invalido em command/query/event e rejeitado pelo contrato.
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

	const FGameplayTagContainer& GetStateTags() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	bool ApplyStatus(FGameplayTag StatusTag, FName SourceId, float DurationOverrideSeconds = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	bool RemoveStatus(FGameplayTag StatusTag, FName SourceId);

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	TArray<FOmniStatusEntry> GetStatusSnapshot() const;

private:
	struct FActiveStatusEffect
	{
		FGameplayTag StatusTag;
		FName SourceId = NAME_None;
		int32 Stacks = 1;
		double StartTimeSeconds = 0.0;
		double ExpireTimeSeconds = 0.0;
		double NextTickTimeSeconds = 0.0;
		float TickIntervalSeconds = 0.0f;
		bool bInfiniteDuration = false;
	};

	void InitializeStatusRecipes();
	void RebuildStatusSnapshot(double NowSeconds);
	bool ResolveClockDelta(float& OutDeltaTime);
	void TickStatusEffects(double NowSeconds);
	int32 FindActiveStatusIndex(FGameplayTag StatusTag, FName SourceId) const;
	void SortActiveStatusEffects();
	void RefreshStatusTags();
	void RefreshCombinedStateTags();
	UOmniAttributesSystem* ResolveAttributesSystem() const;
	void SyncStatusDrivenModifiers(FGameplayTag StatusTag, FName SourceId, bool bActive) const;
	FName BuildStatusModifierSourceId(FGameplayTag StatusTag, FName SourceId) const;
	void PublishTelemetry();

private:
	UPROPERTY(Transient)
	FGameplayTagContainer ContextTags;

	UPROPERTY(Transient)
	FGameplayTagContainer StateTags;

	UPROPERTY(Transient)
	double LastClockSeconds = 0.0;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FOmniStatusRecipe> StatusRecipesByTag;

	UPROPERTY(Transient)
	TArray<FOmniStatusEntry> StatusSnapshot;

	TArray<FActiveStatusEffect> ActiveStatusEffects;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniClockSubsystem> ClockSubsystem;

	UPROPERTY(Transient)
	mutable TWeakObjectPtr<UOmniAttributesSystem> AttributesSystem;
};
