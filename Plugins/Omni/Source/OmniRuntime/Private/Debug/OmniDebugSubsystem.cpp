#include "Debug/OmniDebugSubsystem.h"

#include "Debug/UI/OmniDebugOverlayWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Misc/DateTime.h"

namespace OmniDebug
{
	static const FName CategoryName(TEXT("Debug"));
}
DEFINE_LOG_CATEGORY_STATIC(LogOmniRuntime, Log, All);

void UOmniDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SyncDebugModeFromConsole();
	AddEntry(EOmniDebugLevel::Event, OmniDebug::CategoryName, TEXT("OmniDebugSubsystem inicializado"), TEXT("OmniRuntime"));
}

void UOmniDebugSubsystem::Deinitialize()
{
	HideOverlay();
	AddEntry(EOmniDebugLevel::Event, OmniDebug::CategoryName, TEXT("OmniDebugSubsystem finalizado"), TEXT("OmniRuntime"));
	Super::Deinitialize();
}

void UOmniDebugSubsystem::SetDebugMode(const EOmniDebugMode NewMode)
{
	if (DebugMode == NewMode)
	{
		return;
	}

	DebugMode = NewMode;
	++Revision;
}

void UOmniDebugSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;
	SyncDebugModeFromConsole();

	APlayerController* LocalPlayerController = ResolveLocalPlayerController(nullptr);
	if (!LocalPlayerController || !LocalPlayerController->IsLocalController())
	{
		return;
	}

	if (bEnableF9Clear && LocalPlayerController->WasInputKeyJustPressed(EKeys::F9))
	{
		ClearEntries();
		UE_LOG(LogOmniRuntime, Log, TEXT("[Omni][Runtime][Debug] Entradas de debug limpas via F9 | Atalho de limpeza executado"));
	}

	if (bEnableF10Toggle && LocalPlayerController->WasInputKeyJustPressed(EKeys::F10))
	{
		ToggleOverlay(LocalPlayerController);
	}
}

TStatId UOmniDebugSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOmniDebugSubsystem, STATGROUP_Tickables);
}

bool UOmniDebugSubsystem::IsTickable() const
{
	return !IsTemplate();
}

bool UOmniDebugSubsystem::IsTickableWhenPaused() const
{
	return true;
}

UWorld* UOmniDebugSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

EOmniDebugMode UOmniDebugSubsystem::GetDebugMode() const
{
	return DebugMode;
}

void UOmniDebugSubsystem::SetMaxEntries(const int32 NewMaxEntries)
{
	MaxEntries = FMath::Max(10, NewMaxEntries);
	TrimToMaxEntries();
	++Revision;
}

int32 UOmniDebugSubsystem::GetMaxEntries() const
{
	return MaxEntries;
}

void UOmniDebugSubsystem::ClearEntries()
{
	Entries.Reset();
	++Revision;
}

int32 UOmniDebugSubsystem::GetEntryCount() const
{
	return Entries.Num();
}

int32 UOmniDebugSubsystem::GetRevision() const
{
	return Revision;
}

void UOmniDebugSubsystem::AddEntry(const EOmniDebugLevel Level, const FName Category, const FString& Message, const FName Source)
{
	if (DebugMode == EOmniDebugMode::Off)
	{
		return;
	}
	if (DebugMode == EOmniDebugMode::Basic && Level == EOmniDebugLevel::Info)
	{
		return;
	}

	FOmniDebugEntry Entry;
	Entry.TimestampUtc = BuildTimestampUtc();
	Entry.Level = Level;
	Entry.Category = Category;
	Entry.Source = Source;
	Entry.Message = Message;

	if (const UWorld* World = GetWorld())
	{
		Entry.WorldTimeSeconds = World->GetTimeSeconds();
	}

	Entries.Add(MoveTemp(Entry));
	TrimToMaxEntries();
	++Revision;
}

void UOmniDebugSubsystem::LogInfo(const FName Category, const FString& Message, const FName Source)
{
	AddEntry(EOmniDebugLevel::Info, Category, Message, Source);
}

void UOmniDebugSubsystem::LogWarning(const FName Category, const FString& Message, const FName Source)
{
	AddEntry(EOmniDebugLevel::Warning, Category, Message, Source);
}

void UOmniDebugSubsystem::LogError(const FName Category, const FString& Message, const FName Source)
{
	AddEntry(EOmniDebugLevel::Error, Category, Message, Source);
}

void UOmniDebugSubsystem::LogEvent(const FName Category, const FString& Message, const FName Source)
{
	AddEntry(EOmniDebugLevel::Event, Category, Message, Source);
}

TArray<FOmniDebugEntry> UOmniDebugSubsystem::GetEntriesSnapshot() const
{
	return Entries;
}

TArray<FOmniDebugEntry> UOmniDebugSubsystem::GetRecentEntries(const int32 MaxCount) const
{
	if (MaxCount <= 0 || Entries.Num() == 0)
	{
		return {};
	}

	const int32 SafeCount = FMath::Min(MaxCount, Entries.Num());
	const int32 StartIndex = Entries.Num() - SafeCount;

	TArray<FOmniDebugEntry> Result;
	Result.Reserve(SafeCount);

	for (int32 Index = StartIndex; Index < Entries.Num(); ++Index)
	{
		Result.Add(Entries[Index]);
	}

	return Result;
}

void UOmniDebugSubsystem::SetMetric(const FName Key, const FString& Value)
{
	if (Key == NAME_None)
	{
		return;
	}

	FString* ExistingValue = LiveMetrics.Find(Key);
	if (ExistingValue && *ExistingValue == Value)
	{
		return;
	}

	LiveMetrics.Add(Key, Value);
	++Revision;
}

void UOmniDebugSubsystem::RemoveMetric(const FName Key)
{
	if (Key == NAME_None)
	{
		return;
	}

	if (LiveMetrics.Remove(Key) > 0)
	{
		++Revision;
	}
}

void UOmniDebugSubsystem::ClearMetrics()
{
	if (LiveMetrics.Num() == 0)
	{
		return;
	}

	LiveMetrics.Reset();
	++Revision;
}

TArray<FOmniDebugMetric> UOmniDebugSubsystem::GetMetricsSnapshot() const
{
	TArray<FOmniDebugMetric> Result;
	Result.Reserve(LiveMetrics.Num());

	for (const TPair<FName, FString>& Pair : LiveMetrics)
	{
		FOmniDebugMetric& Metric = Result.AddDefaulted_GetRef();
		Metric.Key = Pair.Key;
		Metric.Value = Pair.Value;
	}

	Result.Sort(
		[](const FOmniDebugMetric& Left, const FOmniDebugMetric& Right)
		{
			return FNameLexicalLess()(Left.Key, Right.Key);
		}
	);

	return Result;
}

bool UOmniDebugSubsystem::ShowOverlay(APlayerController* PlayerController)
{
	if (ActiveOverlayWidget && ActiveOverlayWidget->IsInViewport())
	{
		return true;
	}

	APlayerController* ResolvedPlayerController = ResolveLocalPlayerController(PlayerController);
	if (!ResolvedPlayerController)
	{
		LogWarning(OmniDebug::CategoryName, TEXT("Nao foi possivel exibir o overlay: nenhum PlayerController local"), TEXT("OmniRuntime"));
		return false;
	}

	UClass* WidgetClass = OverlayWidgetClass ? OverlayWidgetClass.Get() : UOmniDebugOverlayWidget::StaticClass();
	if (!WidgetClass)
	{
		LogError(OmniDebug::CategoryName, TEXT("Nao foi possivel exibir o overlay: classe de widget invalida"), TEXT("OmniRuntime"));
		return false;
	}

	UOmniDebugOverlayWidget* NewOverlayWidget = CreateWidget<UOmniDebugOverlayWidget>(ResolvedPlayerController, WidgetClass);
	if (!NewOverlayWidget)
	{
		LogError(OmniDebug::CategoryName, TEXT("Nao foi possivel exibir o overlay: falha ao criar widget"), TEXT("OmniRuntime"));
		return false;
	}

	NewOverlayWidget->InitializeWithSubsystem(this);
	NewOverlayWidget->AddToViewport(9999);
	ActiveOverlayWidget = NewOverlayWidget;
	LogEvent(OmniDebug::CategoryName, TEXT("Overlay de debug exibido"), TEXT("OmniRuntime"));
	return true;
}

void UOmniDebugSubsystem::HideOverlay()
{
	if (!ActiveOverlayWidget)
	{
		return;
	}

	ActiveOverlayWidget->RemoveFromParent();
	ActiveOverlayWidget = nullptr;
	LogEvent(OmniDebug::CategoryName, TEXT("Overlay de debug ocultado"), TEXT("OmniRuntime"));
}

bool UOmniDebugSubsystem::ToggleOverlay(APlayerController* PlayerController)
{
	if (IsOverlayVisible())
	{
		HideOverlay();
		return false;
	}

	return ShowOverlay(PlayerController);
}

bool UOmniDebugSubsystem::IsOverlayVisible() const
{
	return ActiveOverlayWidget && ActiveOverlayWidget->IsInViewport();
}

void UOmniDebugSubsystem::SetOverlayWidgetClass(TSubclassOf<UOmniDebugOverlayWidget> NewOverlayWidgetClass)
{
	OverlayWidgetClass = NewOverlayWidgetClass;
}

void UOmniDebugSubsystem::TrimToMaxEntries()
{
	if (Entries.Num() <= MaxEntries)
	{
		return;
	}

	const int32 OverflowCount = Entries.Num() - MaxEntries;
	Entries.RemoveAt(0, OverflowCount, EAllowShrinking::No);
}

void UOmniDebugSubsystem::SyncDebugModeFromConsole()
{
	IConsoleVariable* DebugModeCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("omni.debug"));
	if (!DebugModeCVar)
	{
		return;
	}

	const int32 CurrentConsoleValue = DebugModeCVar->GetInt();
	if (CurrentConsoleValue == LastAppliedConsoleMode)
	{
		return;
	}

	LastAppliedConsoleMode = CurrentConsoleValue;
	SetDebugMode(ToDebugMode(CurrentConsoleValue));
	if (CurrentConsoleValue > 0)
	{
		UE_LOG(
			LogOmniRuntime,
			Warning,
			TEXT("[Omni][DevMode][Debug] Enabled: omni.debug=%d | Not for production"),
			CurrentConsoleValue
		);
	}
}

EOmniDebugMode UOmniDebugSubsystem::ToDebugMode(const int32 ConsoleValue)
{
	switch (ConsoleValue)
	{
	case 0:
		return EOmniDebugMode::Off;
	case 2:
		return EOmniDebugMode::Full;
	case 1:
	default:
		return EOmniDebugMode::Basic;
	}
}

FString UOmniDebugSubsystem::BuildTimestampUtc()
{
	return FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
}

APlayerController* UOmniDebugSubsystem::ResolveLocalPlayerController(APlayerController* Preferred) const
{
	if (Preferred)
	{
		return Preferred;
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetFirstLocalPlayerController();
	}

	return nullptr;
}
