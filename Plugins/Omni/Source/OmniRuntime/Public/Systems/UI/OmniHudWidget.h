#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Attributes/OmniAttributeTypes.h"
#include "OmniHudWidget.generated.h"

class UOmniUIBridgeSubsystem;
class UProgressBar;
class UTextBlock;

UCLASS()
class OMNIRUNTIME_API UOmniHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Omni|UI|HUD")
	void SetBridgeSubsystem(UOmniUIBridgeSubsystem* InBridgeSubsystem);

	UFUNCTION(BlueprintCallable, Category = "Omni|UI|HUD")
	void RefreshNow();

	UFUNCTION(BlueprintCallable, Category = "Omni|UI|HUD")
	bool ValidateRequiredBindings(FString& OutMissingWidgets);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ResolveBindingsFromWidgetTree();
	bool HasAllRequiredBindings() const;
	void CollectMissingBindingNames(TArray<FString, TInlineAllocator<3>>& OutMissingWidgets) const;
	void EnsureWidgetTreeBuilt();
	void ApplySnapshotToWidgets(const FOmniAttributeSnapshot& Snapshot);

private:
	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HP = nullptr;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Stamina = nullptr;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Exhausted = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UOmniUIBridgeSubsystem> BridgeSubsystem = nullptr;

	bool bLoggedMissingBridge = false;
	bool bNativeFallbackBuilt = false;
};
