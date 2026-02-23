#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "OmniSystemRegistrySubsystem.generated.h"

class UOmniManifest;
class UOmniRuntimeSystem;

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

private:
	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry")
	FSoftObjectPath AutoManifestAssetPath;

	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry")
	FSoftClassPath AutoManifestClassPath;

	UPROPERTY(Config, EditAnywhere, Category = "Omni|Registry")
	bool bUseConfiguredFallbackSystems = true;

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
