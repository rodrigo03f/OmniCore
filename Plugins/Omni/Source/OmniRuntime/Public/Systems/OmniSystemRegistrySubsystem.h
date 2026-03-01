#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Systems/OmniSystemMessaging.h"
#include "UObject/SoftObjectPath.h"
#include "OmniSystemRegistrySubsystem.generated.h"

class UOmniManifest;
class UOmniRuntimeSystem;
class UOmniDebugSubsystem;

// Purpose:
// - Bootstrapar o runtime Omni a partir de UOmniManifest (asset/class configurado).
// - Resolver dependencias declaradas entre systems e inicializar na ordem correta.
// - Rotear Commands/Queries/Events entre systems sem acoplamento direto.
// Inputs:
// - Config de projeto (AutoManifest, flags DEV/fallback).
// - Definicoes de systems/dependencias no Manifest.
// - Mensagens de runtime (Command/Query/Event).
// Outputs:
// - Systems ativos registrados por SystemId.
// - Estado do registry (inicializado/nao inicializado) e manifest ativo.
// - Roteamento de mensagens para o system de destino.
// Determinism:
// - Ordem de inicializacao derivada de dependencias + ordenacao lexical de fila.
// - GetActiveSystemIds retorna ids ordenados.
// Failure modes:
// - Manifest invalido/ausente no fluxo normal => fail-fast com log acionavel.
// - Dependencia circular ou SystemId duplicado => aborta inicializacao.
// - Fallback estrutural so via opt-in DEV explicito.
UCLASS(Config = Game)
class OMNIRUNTIME_API UOmniSystemRegistrySubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "Omni|Registry")
	bool InitializeFromManifest(UOmniManifest* Manifest);

	UFUNCTION(BlueprintCallable, Category = "Omni|Registry")
	void ShutdownSystems();

	UFUNCTION(BlueprintPure, Category = "Omni|Registry")
	bool IsRegistryInitialized() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Registry")
	TArray<FName> GetActiveSystemIds() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Registry")
	UOmniRuntimeSystem* GetSystemById(FName SystemId) const;

	UFUNCTION(BlueprintPure, Category = "Omni|Registry")
	UOmniManifest* GetActiveManifest() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Registry|Messaging")
	bool DispatchCommand(const FOmniCommandMessage& Command);

	UFUNCTION(BlueprintCallable, Category = "Omni|Registry|Messaging")
	bool ExecuteQuery(UPARAM(ref) FOmniQueryMessage& Query);

	UFUNCTION(BlueprintCallable, Category = "Omni|Registry|Messaging")
	void BroadcastEvent(const FOmniEventMessage& Event);

	UFUNCTION(BlueprintPure, Category = "Omni|Registry")
	bool IsDevDefaultsEnabled() const;

private:
	struct FResolvedSystemSpec
	{
		FName SystemId = NAME_None;
		UClass* SystemClass = nullptr;
		TArray<FName> Dependencies;
	};

	bool TryInitializeFromAutoManifest();
	bool TryInitializeFromConfiguredFallback();
	bool BuildSpecs(const UOmniManifest* Manifest, TMap<FName, FResolvedSystemSpec>& OutSpecs) const;
	bool BuildInitializationOrder(
		const TMap<FName, FResolvedSystemSpec>& Specs,
		TArray<FName>& OutInitializationOrder
	) const;
	void ShutdownSystemsInternal(bool bLogSummary);
	UOmniDebugSubsystem* TryGetDebugSubsystem() const;
	void PublishRegistryDiagnostics(bool bManifestLoaded) const;

private:
	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry")
	FSoftObjectPath AutoManifestAssetPath;

	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry")
	FSoftClassPath AutoManifestClassPath;

	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry")
	bool bUseConfiguredFallbackSystems = false;

	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry|Dev")
	bool bAllowDevDefaults = false;

	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry")
	TArray<FSoftClassPath> FallbackSystemClasses;

	UPROPERTY(Transient)
	TObjectPtr<UOmniManifest> ActiveManifest = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOmniRuntimeSystem>> ActiveSystems;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UOmniRuntimeSystem>> SystemsById;

	UPROPERTY(Transient)
	bool bRegistryInitialized = false;
};
