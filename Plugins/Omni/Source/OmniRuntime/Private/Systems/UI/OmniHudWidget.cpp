#include "Systems/UI/OmniHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Systems/UI/OmniUIBridgeSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniHudWidget, Log, All);

void UOmniHudWidget::ResolveBindingsFromWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!PB_HP)
	{
		PB_HP = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("PB_HP")));
	}

	if (!PB_Stamina)
	{
		PB_Stamina = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("PB_Stamina")));
	}

	if (!TXT_Exhausted)
	{
		TXT_Exhausted = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TXT_Exhausted")));
	}
}

bool UOmniHudWidget::HasAllRequiredBindings() const
{
	return PB_HP && PB_Stamina && TXT_Exhausted;
}

void UOmniHudWidget::CollectMissingBindingNames(TArray<FString, TInlineAllocator<3>>& OutMissingWidgets) const
{
	if (!PB_HP)
	{
		OutMissingWidgets.Add(TEXT("PB_HP"));
	}

	if (!PB_Stamina)
	{
		OutMissingWidgets.Add(TEXT("PB_Stamina"));
	}

	if (!TXT_Exhausted)
	{
		OutMissingWidgets.Add(TEXT("TXT_Exhausted"));
	}
}

bool UOmniHudWidget::ValidateRequiredBindings(FString& OutMissingWidgets)
{
	ResolveBindingsFromWidgetTree();

	TArray<FString, TInlineAllocator<3>> MissingWidgets;
	CollectMissingBindingNames(MissingWidgets);
	OutMissingWidgets = FString::Join(MissingWidgets, TEXT(","));
	return MissingWidgets.Num() == 0;
}

void UOmniHudWidget::SetBridgeSubsystem(UOmniUIBridgeSubsystem* InBridgeSubsystem)
{
	BridgeSubsystem = InBridgeSubsystem;
	bLoggedMissingBridge = false;
	RefreshNow();
}

void UOmniHudWidget::RefreshNow()
{
	EnsureWidgetTreeBuilt();

	if (!BridgeSubsystem)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			BridgeSubsystem = GameInstance->GetSubsystem<UOmniUIBridgeSubsystem>();
		}
	}

	if (!BridgeSubsystem)
	{
		if (!bLoggedMissingBridge)
		{
			bLoggedMissingBridge = true;
			UE_LOG(LogOmniHudWidget, Warning, TEXT("[Omni][UI][HUD] Bridge indisponivel para atualizar HUD"));
		}
		return;
	}

	bLoggedMissingBridge = false;

	FOmniAttributeSnapshot Snapshot;
	if (!BridgeSubsystem->GetLatestAttributeSnapshot(Snapshot))
	{
		return;
	}

	ApplySnapshotToWidgets(Snapshot);
}

void UOmniHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureWidgetTreeBuilt();
	RefreshNow();
}

void UOmniHudWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshNow();
}

void UOmniHudWidget::EnsureWidgetTreeBuilt()
{
	if (!WidgetTree)
	{
		return;
	}

	ResolveBindingsFromWidgetTree();
	if (HasAllRequiredBindings())
	{
		return;
	}

	const bool bIsNativeOnlyWidget = (GetClass() == StaticClass());
	if (!bIsNativeOnlyWidget)
	{
		return;
	}

	if (bNativeFallbackBuilt)
	{
		return;
	}

	bNativeFallbackBuilt = true;

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HUDRoot"));
	RootBorder->SetPadding(FMargin(12.0f));
	RootBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));

	UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HUDVBox"));

	UTextBlock* HPLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HP_Label"));
	HPLabel->SetText(FText::FromString(TEXT("HP")));
	PB_HP = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PB_HP"));
	PB_HP->SetPercent(1.0f);

	UTextBlock* StaminaLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Stamina_Label"));
	StaminaLabel->SetText(FText::FromString(TEXT("Stamina")));
	PB_Stamina = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PB_Stamina"));
	PB_Stamina->SetPercent(1.0f);

	TXT_Exhausted = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_Exhausted"));
	TXT_Exhausted->SetText(FText::FromString(TEXT("EXHAUSTED")));
	TXT_Exhausted->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f, 1.0f)));
	TXT_Exhausted->SetVisibility(ESlateVisibility::Collapsed);

	auto AddWithPadding = [VerticalBox](UWidget* Widget, const FMargin& SlotPadding)
	{
		if (UVerticalBoxSlot* Slot = VerticalBox->AddChildToVerticalBox(Widget))
		{
			Slot->SetPadding(SlotPadding);
		}
	};

	AddWithPadding(HPLabel, FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	AddWithPadding(PB_HP, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	AddWithPadding(StaminaLabel, FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	AddWithPadding(PB_Stamina, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	AddWithPadding(TXT_Exhausted, FMargin(0.0f));

	RootBorder->SetContent(VerticalBox);
	WidgetTree->RootWidget = RootBorder;

	UE_LOG(
		LogOmniHudWidget,
		Warning,
		TEXT("[Omni][UI][HUD] Usando fallback de layout C++ (classe nativa sem UMG associado).")
	);
}

void UOmniHudWidget::ApplySnapshotToWidgets(const FOmniAttributeSnapshot& Snapshot)
{
	if (!PB_HP || !PB_Stamina || !TXT_Exhausted)
	{
		return;
	}

	const float HpDenominator = FMath::Max(Snapshot.HP.Max, 1.0f);
	const float StaminaDenominator = FMath::Max(Snapshot.Stamina.Max, 1.0f);

	const float HpPercent = FMath::Clamp(Snapshot.HP.Current / HpDenominator, 0.0f, 1.0f);
	const float StaminaPercent = FMath::Clamp(Snapshot.Stamina.Current / StaminaDenominator, 0.0f, 1.0f);

	PB_HP->SetPercent(HpPercent);
	PB_Stamina->SetPercent(StaminaPercent);
	TXT_Exhausted->SetVisibility(Snapshot.bExhausted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
