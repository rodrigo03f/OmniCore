#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Systems/Attributes/OmniAttributeTypes.h"
#include "OmniUIBridgeSubsystem.generated.h"

class APlayerController;
class UOmniHudWidget;

UCLASS()
class OMNIRUNTIME_API UOmniUIBridgeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	UFUNCTION(BlueprintPure, Category = "Omni|UI|HUD")
	bool GetLatestAttributeSnapshot(FOmniAttributeSnapshot& OutSnapshot) const;

	UFUNCTION(BlueprintPure, Category = "Omni|UI|HUD")
	bool HasLatestAttributeSnapshot() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|UI|HUD")
	bool EnsureHudCreated();

	UFUNCTION(BlueprintCallable, Category = "Omni|UI|HUD")
	void RemoveHud();

	UFUNCTION(BlueprintPure, Category = "Omni|UI|HUD")
	bool IsHudVisible() const;

private:
	enum class EHudWidgetConfigState : uint8
	{
		Unknown,
		Valid,
		MissingWBP,
		InvalidClass
	};

	void ResetState();
	void ApplyConfigOverrides();
	bool RefreshCachedSnapshot();
	APlayerController* ResolveLocalPlayerController() const;
	UClass* ResolveHudWidgetClass(FString& OutPath, FString& OutFallbackReason);
	void LogFallbackActivated(const FString& Reason, const FString& Path);

private:
	UPROPERTY(EditAnywhere, Category = "Omni|UI|HUD")
	bool bAutoCreateHud = true;

	UPROPERTY(EditAnywhere, Category = "Omni|UI|HUD")
	int32 HudZOrder = 100;

	UPROPERTY(EditAnywhere, Category = "Omni|UI|HUD")
	TSubclassOf<UOmniHudWidget> HudWidgetClass;

	UPROPERTY(Transient)
	FOmniAttributeSnapshot CachedSnapshot;

	UPROPERTY(Transient)
	bool bHasCachedSnapshot = false;

	UPROPERTY(Transient)
	TObjectPtr<UOmniHudWidget> ActiveHudWidget = nullptr;

	UPROPERTY(Transient)
	bool bLoggedMissingStatusSystem = false;

	UPROPERTY(Transient)
	bool bLoggedWidgetClassLoadFailure = false;

	UPROPERTY(Transient)
	bool bLoggedWidgetCreationFailure = false;

	bool bLoggedFallbackActivated = false;

	FString ConfiguredWidgetClassPath;

	EHudWidgetConfigState HudWidgetConfigState = EHudWidgetConfigState::Unknown;
};
