# OmniRuntime / OmniDebugOverlayWidget

## Arquivos agrupados
- Plugins/Omni/Source/OmniRuntime/Private/Debug/UI/OmniDebugOverlayWidget.cpp
- Plugins/Omni/Source/OmniRuntime/Public/Debug/UI/OmniDebugOverlayWidget.h

## Header: Plugins/Omni/Source/OmniRuntime/Public/Debug/UI/OmniDebugOverlayWidget.h
```cpp
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

```

## Source: Plugins/Omni/Source/OmniRuntime/Private/Debug/UI/OmniDebugOverlayWidget.cpp
```cpp
#include "Debug/UI/OmniDebugOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "OmniDebugOverlay"

namespace OmniDebugOverlay
{
	static FText ToDebugLevelText(const EOmniDebugLevel Level)
	{
		switch (Level)
		{
		case EOmniDebugLevel::Info:
			return LOCTEXT("LevelInfo", "Info");
		case EOmniDebugLevel::Warning:
			return LOCTEXT("LevelWarning", "Aviso");
		case EOmniDebugLevel::Error:
			return LOCTEXT("LevelError", "Erro");
		case EOmniDebugLevel::Event:
			return LOCTEXT("LevelEvent", "Evento");
		default:
			return LOCTEXT("LevelUnknown", "Desconhecido");
		}
	}

	static FText ToDebugModeText(const EOmniDebugMode Mode)
	{
		switch (Mode)
		{
		case EOmniDebugMode::Off:
			return LOCTEXT("ModeOff", "Desligado");
		case EOmniDebugMode::Basic:
			return LOCTEXT("ModeBasic", "Basico");
		case EOmniDebugMode::Full:
			return LOCTEXT("ModeFull", "Completo");
		default:
			return LOCTEXT("ModeUnknown", "Desconhecido");
		}
	}
}

void UOmniDebugOverlayWidget::InitializeWithSubsystem(UOmniDebugSubsystem* InDebugSubsystem)
{
	DebugSubsystem = InDebugSubsystem;
	LastSeenRevision = INDEX_NONE;
	RefreshNow();
}

void UOmniDebugOverlayWidget::SetMaxVisibleEntries(const int32 NewMaxVisibleEntries)
{
	MaxVisibleEntries = FMath::Max(1, NewMaxVisibleEntries);
	LastSeenRevision = INDEX_NONE;
	RefreshNow();
}

void UOmniDebugOverlayWidget::RefreshNow()
{
	EnsureWidgetTreeBuilt();

	if (!DebugSubsystem)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>();
		}
	}

	if (!HeaderText || !BodyText)
	{
		return;
	}

	if (!DebugSubsystem)
	{
		HeaderText->SetText(LOCTEXT("HeaderSubsystemUnavailable", "Omni Debug Overlay (Subsystem indisponivel)"));
		BodyText->SetText(LOCTEXT("BodyNoDataSource", "Nenhuma fonte de dados encontrada."));
		return;
	}

	const int32 CurrentRevision = DebugSubsystem->GetRevision();
	if (CurrentRevision == LastSeenRevision)
	{
		return;
	}

	HeaderText->SetText(BuildHeaderText());
	BodyText->SetText(BuildBodyText());
	LastSeenRevision = CurrentRevision;
}

void UOmniDebugOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureWidgetTreeBuilt();

	if (bAutoRefresh && RefreshIntervalSeconds > 0.0f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			RefreshTimerHandle,
			this,
			&UOmniDebugOverlayWidget::RefreshNow,
			RefreshIntervalSeconds,
			true
		);
	}

	RefreshNow();
}

void UOmniDebugOverlayWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void UOmniDebugOverlayWidget::EnsureWidgetTreeBuilt()
{
	if (!WidgetTree || (HeaderText && BodyText))
	{
		return;
	}

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OmniDebugRoot"));
	RootBorder->SetPadding(FMargin(8.0f));
	RootBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));

	UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OmniDebugVerticalBox"));

	HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OmniDebugHeader"));
	HeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.95f, 0.4f, 1.0f)));
	{
		FSlateFontInfo HeaderFont = HeaderText->GetFont();
		HeaderFont.Size = HeaderFontSize;
		HeaderText->SetFont(HeaderFont);
	}
	HeaderText->SetText(LOCTEXT("HeaderDefault", "Omni Painel de Debug"));

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OmniDebugBody"));
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	{
		FSlateFontInfo BodyFont = BodyText->GetFont();
		BodyFont.Size = BodyFontSize;
		BodyText->SetFont(BodyFont);
	}
	BodyText->SetAutoWrapText(false);
	BodyText->SetText(LOCTEXT("BodyWaiting", "Aguardando logs..."));

	if (UVerticalBoxSlot* HeaderSlot = VerticalBox->AddChildToVerticalBox(HeaderText))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}
	VerticalBox->AddChildToVerticalBox(BodyText);

	RootBorder->SetContent(VerticalBox);
	WidgetTree->RootWidget = RootBorder;
}

FText UOmniDebugOverlayWidget::BuildHeaderText() const
{
	if (!DebugSubsystem)
	{
		return LOCTEXT("HeaderFallback", "Omni Painel de Debug");
	}

	return FText::Format(
		LOCTEXT("HeaderFormat", "Omni Debug | Modo={0} | Entradas={1}/{2} | Rev={3}"),
		OmniDebugOverlay::ToDebugModeText(DebugSubsystem->GetDebugMode()),
		FText::AsNumber(DebugSubsystem->GetEntryCount()),
		FText::AsNumber(DebugSubsystem->GetMaxEntries()),
		FText::AsNumber(DebugSubsystem->GetRevision())
	);
}

FText UOmniDebugOverlayWidget::BuildBodyText() const
{
	if (!DebugSubsystem)
	{
		return LOCTEXT("BodyNoSubsystem", "Subsystem de debug indisponivel.");
	}

	const TArray<FOmniDebugEntry> RecentEntries = DebugSubsystem->GetRecentEntries(MaxVisibleEntries);
	const TArray<FOmniDebugMetric> Metrics = DebugSubsystem->GetMetricsSnapshot();

	FString Result;

	if (Metrics.Num() > 0)
	{
		Result += LOCTEXT("MetricsHeader", "[Metricas]").ToString();
		Result += TEXT("\n");

		for (const FOmniDebugMetric& Metric : Metrics)
		{
			Result += FString::Printf(TEXT("%s: %s\n"), *Metric.Key.ToString(), *Metric.Value);
		}

		Result += TEXT("\n");
	}

	if (RecentEntries.Num() == 0)
	{
		if (Result.IsEmpty())
		{
			return LOCTEXT("BodyNoEntries", "Sem entradas.");
		}

		return FText::FromString(Result);
	}

	for (const FOmniDebugEntry& Entry : RecentEntries)
	{
		const FText LineText = FText::Format(
			LOCTEXT("BodyLineFormat", "[{0}][{1}][{2}] {3}"),
			FText::FromString(Entry.TimestampUtc),
			OmniDebugOverlay::ToDebugLevelText(Entry.Level),
			FText::FromName(Entry.Category),
			FText::FromString(Entry.Message)
		);
		Result += LineText.ToString();
		Result += TEXT("\n");
	}

	return FText::FromString(Result);
}

#undef LOCTEXT_NAMESPACE

```

