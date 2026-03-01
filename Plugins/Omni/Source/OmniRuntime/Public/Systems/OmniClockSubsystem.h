#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OmniClockSubsystem.generated.h"

// Purpose:
// - Fornecer tempo simulado unificado para o runtime Omni.
// - Expor contador de tick para diagnostico e sincronizacao.
// Inputs:
// - DeltaTime do loop de tick da engine.
// Outputs:
// - SimTimeSeconds e TickIndex consumidos por systems.
// Determinism:
// - Tempo monotono por acumulacao de DeltaTime (clamp >= 0).
// - Fonte unica para systems que exigem tempo deterministico.
// Failure modes:
// - Sem falhas internas previstas; indisponibilidade deve ser tratada por
//   dependentes com fail-fast na inicializacao.
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
	UPROPERTY(Transient)
	double SimTimeSeconds = 0.0;

	UPROPERTY(Transient)
	int64 TickIndex = 0;
};
