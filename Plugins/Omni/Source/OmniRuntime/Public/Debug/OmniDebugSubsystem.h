#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Debug/OmniDebugTypes.h"
#include "OmniDebugSubsystem.generated.h"

class APlayerController;
class UOmniDebugOverlayWidget;

UCLASS()
class OMNIRUNTIME_API UOmniDebugSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void SetDebugMode(EOmniDebugMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Omni|Debug")
	EOmniDebugMode GetDebugMode() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void SetMaxEntries(int32 NewMaxEntries);

	UFUNCTION(BlueprintPure, Category = "Omni|Debug")
	int32 GetMaxEntries() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void ClearEntries();

	UFUNCTION(BlueprintPure, Category = "Omni|Debug")
	int32 GetEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Debug")
	int32 GetRevision() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void AddEntry(EOmniDebugLevel Level, FName Category, const FString& Message, FName Source = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void LogInfo(FName Category, const FString& Message, FName Source = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void LogWarning(FName Category, const FString& Message, FName Source = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void LogError(FName Category, const FString& Message, FName Source = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void LogEvent(FName Category, const FString& Message, FName Source = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Omni|Debug")
	TArray<FOmniDebugEntry> GetEntriesSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Debug")
	TArray<FOmniDebugEntry> GetRecentEntries(int32 MaxCount) const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void SetMetric(FName Key, const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void RemoveMetric(FName Key);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug")
	void ClearMetrics();

	UFUNCTION(BlueprintPure, Category = "Omni|Debug")
	TArray<FOmniDebugMetric> GetMetricsSnapshot() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug|Overlay")
	bool ShowOverlay(APlayerController* PlayerController = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug|Overlay")
	void HideOverlay();

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug|Overlay")
	bool ToggleOverlay(APlayerController* PlayerController = nullptr);

	UFUNCTION(BlueprintPure, Category = "Omni|Debug|Overlay")
	bool IsOverlayVisible() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug|Overlay")
	void SetOverlayWidgetClass(TSubclassOf<UOmniDebugOverlayWidget> NewOverlayWidgetClass);

private:
	void TrimToMaxEntries();
	void SyncDebugModeFromConsole();
	static EOmniDebugMode ToDebugMode(int32 ConsoleValue);
	static FString BuildTimestampUtc();
	APlayerController* ResolveLocalPlayerController(APlayerController* Preferred) const;

private:
	UPROPERTY()
	TArray<FOmniDebugEntry> Entries;

	UPROPERTY()
	TMap<FName, FString> LiveMetrics;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug")
	EOmniDebugMode DebugMode = EOmniDebugMode::Off;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug", meta = (ClampMin = "10", UIMin = "10"))
	int32 MaxEntries = 200;

	UPROPERTY()
	int32 Revision = 0;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay")
	TSubclassOf<UOmniDebugOverlayWidget> OverlayWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UOmniDebugOverlayWidget> ActiveOverlayWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay")
	bool bEnableF10Toggle = true;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay")
	bool bEnableF9Clear = true;

	UPROPERTY(Transient)
	int32 LastAppliedConsoleMode = INDEX_NONE;
};
