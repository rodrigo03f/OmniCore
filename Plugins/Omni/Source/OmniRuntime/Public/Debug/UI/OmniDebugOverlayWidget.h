#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OmniDebugOverlayWidget.generated.h"

class UOmniDebugSubsystem;
class UTextBlock;

UCLASS()
class OMNIRUNTIME_API UOmniDebugOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Omni|Debug|Overlay")
	void InitializeWithSubsystem(UOmniDebugSubsystem* InDebugSubsystem);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug|Overlay")
	void SetMaxVisibleEntries(int32 NewMaxVisibleEntries);

	UFUNCTION(BlueprintCallable, Category = "Omni|Debug|Overlay")
	void RefreshNow();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void EnsureWidgetTreeBuilt();
	FText BuildHeaderText() const;
	FText BuildBodyText() const;

private:
	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay", meta = (ClampMin = "8", UIMin = "8"))
	int32 HeaderFontSize = 12;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay", meta = (ClampMin = "8", UIMin = "8"))
	int32 BodyFontSize = 10;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxVisibleEntries = 20;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float RefreshIntervalSeconds = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Omni|Debug|Overlay")
	bool bAutoRefresh = true;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText = nullptr;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UOmniDebugSubsystem> DebugSubsystem = nullptr;

	FTimerHandle RefreshTimerHandle;
	int32 LastSeenRevision = INDEX_NONE;
};
