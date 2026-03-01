#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/Status/OmniStatusData.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniStatusSystem.generated.h"

class UOmniManifest;
class UOmniDebugSubsystem;
class UOmniSystemRegistrySubsystem;

// Purpose:
// - Manter estado de status do ator (stamina, exhausted e state tags).
// - Processar consumo/regeneracao de stamina conforme config de profile.
// - Responder queries de estado para outros systems.
// Inputs:
// - Config de status (Manifest -> StatusProfile).
// - Commands/Events de systems (ex.: sprint start/stop).
// - Tick do runtime para evolucao temporal.
// Outputs:
// - Valores de stamina/exhausted para consultas.
// - Gameplay tags de estado (ex.: exhausted).
// - Telemetria de debug.
// Determinism:
// - Evolucao de estado depende apenas de config + tick/runtime messages.
// - Sem uso de random ou fontes de tempo externas no caminho normal.
// Failure modes:
// - Profile invalido/ausente => fail-fast com mensagem acionavel.
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
	bool TryLoadSettingsFromManifest(const UOmniManifest* Manifest, FString& OutError);
	void UpdateStateTags();
	void PublishTelemetry();

private:
	UPROPERTY(Transient)
	FOmniStatusSettings RuntimeSettings;

	UPROPERTY(Transient)
	float CurrentStamina = 0.0f;

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
