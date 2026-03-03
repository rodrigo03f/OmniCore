#pragma once

#include "CoreMinimal.h"
#include "Systems/Modifiers/OmniModifiersData.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniModifiersSystem.generated.h"

class UOmniDebugSubsystem;

// Purpose:
// - Manter pipeline deterministico de modificadores por TargetTag.
// - Expor Add/Remove/Evaluate/Snapshot para consumo por outros systems via contrato.
// Inputs:
// - Commands AddModifier/RemoveModifier.
// - Queries EvaluateModifier/GetModifiersSnapshotCsv.
// Outputs:
// - Resultado deterministico de Evaluate.
// - Snapshot ordenado por TargetTag, ModifierTag, SourceId.
// Determinism:
// - Ordenacao estavel e pipeline fixa: base + adds + muls.
// Failure modes:
// - Payload invalido rejeitado com retorno false e log acionavel.
UCLASS()
class OMNIRUNTIME_API UOmniModifiersSystem : public UOmniRuntimeSystem
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

	UFUNCTION(BlueprintCallable, Category = "Omni|Modifiers")
	bool AddModifier(const FOmniModifierSpec& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Omni|Modifiers")
	bool RemoveModifier(FGameplayTag ModifierTag, FName SourceId);

	UFUNCTION(BlueprintPure, Category = "Omni|Modifiers")
	float Evaluate(FGameplayTag TargetTag, float BaseValue) const;

	UFUNCTION(BlueprintPure, Category = "Omni|Modifiers")
	TArray<FOmniModifierSpec> GetSnapshot() const;

private:
	void SortModifiers();
	void PublishTelemetry() const;
	int32 FindModifierIndex(FGameplayTag ModifierTag, FName SourceId) const;

private:
	UPROPERTY(Transient)
	TArray<FOmniModifierSpec> ActiveModifiers;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;
};
