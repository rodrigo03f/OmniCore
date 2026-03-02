#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/Attributes/OmniAttributeTypes.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniStatusSystem.generated.h"

class UOmniManifest;
class UOmniDebugSubsystem;
class UOmniSystemRegistrySubsystem;
class UOmniClockSubsystem;
class UOmniAttributesRecipeDataAsset;

// Purpose:
// - Manter atributos essenciais (HP/Stamina) de forma deterministica.
// - Processar loop de stamina (drain/regen/exhausted) via tags de contexto.
// - Responder queries de estado para outros systems.
// Inputs:
// - Recipe de atributos via config (DefaultGame.ini).
// - Commands de systems (ex.: SetSprinting) e console (damage/heal).
// - Tempo de simulacao via OmniClock.
// Outputs:
// - Snapshot de atributos (HP/Stamina/Exhausted).
// - Gameplay tags de estado (Game.State.Exhausted).
// - Telemetria de debug.
// Determinism:
// - Evolucao de estado depende de recipe + tags + OmniClock.
// - Sem uso de tempo de mundo no fluxo normal.
// Failure modes:
// - Recipe invalida/ausente => fallback deterministico com log acionavel.
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

	UFUNCTION(BlueprintPure, Category = "Omni|Attributes")
	FOmniAttributeSnapshot GetSnapshot() const;

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

	UFUNCTION(BlueprintCallable, Category = "Omni|Attributes")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Omni|Attributes")
	void ApplyHeal(float Amount);

private:
	bool TryLoadRecipeFromConfig(FString& OutError);
	void ApplyFallbackRecipe();
	void ResolveOfficialTags();
	void InitializeSnapshot();
	bool ResolveClockDelta(float& OutDeltaTime);
	void TickStamina(const float DeltaTime);
	FOmniAttributeValue* FindAttributeMutable(FGameplayTag AttributeTag);
	const FOmniAttributeValue* FindAttribute(FGameplayTag AttributeTag) const;
	void ClampAttribute(FOmniAttributeValue& Attribute) const;
	void SetExhausted(bool bInExhausted);
	void UpdateStateTags();
	void PublishTelemetry();

private:
	// Core deterministic state map consumed by HUD/debug/query.
	TMap<FGameplayTag, FOmniAttributeValue> AttributesByTag;

	UPROPERTY(Transient)
	FOmniStaminaRules StaminaRules;

	UPROPERTY(Transient)
	FOmniAttributeSnapshot Snapshot;

	UPROPERTY(Transient)
	FGameplayTagContainer ContextTags;

	UPROPERTY(Transient)
	FGameplayTagContainer StateTags;

	UPROPERTY(Transient)
	bool bExhausted = false;

	UPROPERTY(Transient)
	double LastClockSeconds = 0.0;

	UPROPERTY(Transient)
	float TimeSinceLastStaminaDrain = 0.0f;

	UPROPERTY(Transient)
	FGameplayTag HpTag;

	UPROPERTY(Transient)
	FGameplayTag StaminaTag;

	UPROPERTY(Transient)
	FGameplayTag SprintingStateTag;

	UPROPERTY(Transient)
	FGameplayTag ExhaustedStateTag;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniClockSubsystem> ClockSubsystem;
};
