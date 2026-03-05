#include "Systems/UI/OmniUIBridgeSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/Status/OmniStatusSystem.h"
#include "Systems/UI/OmniHudWidget.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniUIBridgeSubsystem, Log, All);

namespace OmniUIBridge
{
	static const FName ConfigSection(TEXT("Omni.UI.HUD"));
	static const FName ConfigKeyWidgetClass(TEXT("WidgetClass"));
	static const TCHAR* CanonicalWidgetClassPath = TEXT("/Game/Data/Omni/UI/WBP_OmniHUD_Minimal.WBP_OmniHUD_Minimal_C");
}

void UOmniUIBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetState();
	ApplyConfigOverrides();

	UE_LOG(
		LogOmniUIBridgeSubsystem,
		Log,
		TEXT("[Omni][UI][HUD] Startup widget path (source-of-truth): %s"),
		ConfiguredWidgetClassPath.IsEmpty() ? TEXT("<unset>") : *ConfiguredWidgetClassPath
	);
	UE_LOG(LogOmniUIBridgeSubsystem, Log, TEXT("[Omni][UI][HUD] Bridge inicializado"));
}

void UOmniUIBridgeSubsystem::Deinitialize()
{
	RemoveHud();
	ResetState();

	UE_LOG(LogOmniUIBridgeSubsystem, Log, TEXT("[Omni][UI][HUD] Bridge finalizado"));
	Super::Deinitialize();
}

void UOmniUIBridgeSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;

	RefreshCachedSnapshot();
	if (bAutoCreateHud)
	{
		EnsureHudCreated();
	}
}

TStatId UOmniUIBridgeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOmniUIBridgeSubsystem, STATGROUP_Tickables);
}

bool UOmniUIBridgeSubsystem::IsTickable() const
{
	return !IsTemplate();
}

UWorld* UOmniUIBridgeSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

bool UOmniUIBridgeSubsystem::GetLatestAttributeSnapshot(FOmniAttributeSnapshot& OutSnapshot) const
{
	OutSnapshot = CachedSnapshot;
	return bHasCachedSnapshot;
}

bool UOmniUIBridgeSubsystem::HasLatestAttributeSnapshot() const
{
	return bHasCachedSnapshot;
}

bool UOmniUIBridgeSubsystem::EnsureHudCreated()
{
	if (ActiveHudWidget && ActiveHudWidget->IsInViewport())
	{
		return true;
	}

	APlayerController* LocalPlayerController = ResolveLocalPlayerController();
	if (!LocalPlayerController || !LocalPlayerController->IsLocalController())
	{
		return false;
	}

	FString WidgetPath;
	FString FallbackReason;
	UClass* WidgetClass = ResolveHudWidgetClass(WidgetPath, FallbackReason);
	if (!FallbackReason.IsEmpty())
	{
		LogFallbackActivated(FallbackReason, WidgetPath);
	}

	if (!WidgetClass)
	{
		if (!bLoggedWidgetClassLoadFailure)
		{
			bLoggedWidgetClassLoadFailure = true;
			UE_LOG(
				LogOmniUIBridgeSubsystem,
				Warning,
				TEXT("[Omni][UI][HUD] Classe de widget invalida | Configure [Omni.UI.HUD] WidgetClass ou use default C++")
			);
		}
		return false;
	}

	UOmniHudWidget* NewHudWidget = CreateWidget<UOmniHudWidget>(LocalPlayerController, WidgetClass);
	if (!NewHudWidget)
	{
		if (!bLoggedWidgetCreationFailure)
		{
			bLoggedWidgetCreationFailure = true;
			UE_LOG(LogOmniUIBridgeSubsystem, Warning, TEXT("[Omni][UI][HUD] Falha ao criar widget da HUD"));
		}
		return false;
	}

	if (WidgetClass != UOmniHudWidget::StaticClass())
	{
		FString MissingBindings;
		if (!NewHudWidget->ValidateRequiredBindings(MissingBindings))
		{
			LogFallbackActivated(TEXT("BindMissing"), WidgetPath);

			UE_LOG(
				LogOmniUIBridgeSubsystem,
				Warning,
				TEXT("[Omni][UI][HUD] BindWidget missing no widget configurado (%s) | missing=%s"),
				*WidgetPath,
				MissingBindings.IsEmpty() ? TEXT("<unknown>") : *MissingBindings
			);

			NewHudWidget->RemoveFromParent();
			NewHudWidget = CreateWidget<UOmniHudWidget>(LocalPlayerController, UOmniHudWidget::StaticClass());
			WidgetClass = UOmniHudWidget::StaticClass();

			if (!NewHudWidget)
			{
				if (!bLoggedWidgetCreationFailure)
				{
					bLoggedWidgetCreationFailure = true;
					UE_LOG(LogOmniUIBridgeSubsystem, Warning, TEXT("[Omni][UI][HUD] Falha ao criar widget da HUD em fallback"));
				}
				return false;
			}
		}
	}

	NewHudWidget->SetBridgeSubsystem(this);
	NewHudWidget->AddToViewport(HudZOrder);
	ActiveHudWidget = NewHudWidget;
	bLoggedWidgetCreationFailure = false;

	UE_LOG(
		LogOmniUIBridgeSubsystem,
		Log,
		TEXT("[Omni][UI][HUD] HUD criada e adicionada ao viewport | class=%s z=%d"),
		*WidgetClass->GetPathName(),
		HudZOrder
	);

	return true;
}

void UOmniUIBridgeSubsystem::RemoveHud()
{
	if (!ActiveHudWidget)
	{
		return;
	}

	ActiveHudWidget->RemoveFromParent();
	ActiveHudWidget = nullptr;
}

bool UOmniUIBridgeSubsystem::IsHudVisible() const
{
	return ActiveHudWidget && ActiveHudWidget->IsInViewport();
}

void UOmniUIBridgeSubsystem::ResetState()
{
	CachedSnapshot = FOmniAttributeSnapshot();
	bHasCachedSnapshot = false;
	ActiveHudWidget = nullptr;
	bLoggedMissingStatusSystem = false;
	bLoggedWidgetClassLoadFailure = false;
	bLoggedWidgetCreationFailure = false;
	bLoggedFallbackActivated = false;
	ConfiguredWidgetClassPath = OmniUIBridge::CanonicalWidgetClassPath;
	HudWidgetConfigState = EHudWidgetConfigState::Unknown;
}

void UOmniUIBridgeSubsystem::ApplyConfigOverrides()
{
	HudWidgetClass = nullptr;
	HudWidgetConfigState = EHudWidgetConfigState::Unknown;

	FString WidgetClassPathValue;
	const bool bHasConfigValue = GConfig->GetString(
		*OmniUIBridge::ConfigSection.ToString(),
		*OmniUIBridge::ConfigKeyWidgetClass.ToString(),
		WidgetClassPathValue,
		GGameIni
	);

	if (!bHasConfigValue)
	{
		WidgetClassPathValue = OmniUIBridge::CanonicalWidgetClassPath;
	}

	WidgetClassPathValue.TrimStartAndEndInline();
	ConfiguredWidgetClassPath = WidgetClassPathValue;
	if (WidgetClassPathValue.IsEmpty())
	{
		HudWidgetConfigState = EHudWidgetConfigState::MissingWBP;
		return;
	}

	const FSoftClassPath WidgetClassPath(WidgetClassPathValue);
	if (WidgetClassPath.IsNull())
	{
		HudWidgetConfigState = EHudWidgetConfigState::InvalidClass;
		UE_LOG(
			LogOmniUIBridgeSubsystem,
			Warning,
			TEXT("[Omni][UI][HUD] WidgetClass invalido em config: %s"),
			*WidgetClassPathValue
		);
		return;
	}

	UClass* LoadedWidgetClass = WidgetClassPath.TryLoadClass<UUserWidget>();
	if (!LoadedWidgetClass)
	{
		HudWidgetConfigState = EHudWidgetConfigState::MissingWBP;
		UE_LOG(
			LogOmniUIBridgeSubsystem,
			Warning,
			TEXT("[Omni][UI][HUD] WidgetClass nao carregado: %s"),
			*WidgetClassPath.ToString()
		);
		return;
	}

	if (!LoadedWidgetClass->IsChildOf(UOmniHudWidget::StaticClass()))
	{
		HudWidgetConfigState = EHudWidgetConfigState::InvalidClass;
		UE_LOG(
			LogOmniUIBridgeSubsystem,
			Warning,
			TEXT("[Omni][UI][HUD] WidgetClass nao herda UOmniHudWidget: %s"),
			*LoadedWidgetClass->GetPathName()
		);
		return;
	}

	HudWidgetClass = LoadedWidgetClass;
	HudWidgetConfigState = EHudWidgetConfigState::Valid;
	UE_LOG(
		LogOmniUIBridgeSubsystem,
		Log,
		TEXT("[Omni][UI][HUD] WidgetClass carregado de config: %s"),
		*LoadedWidgetClass->GetPathName()
	);
}

bool UOmniUIBridgeSubsystem::RefreshCachedSnapshot()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	UOmniSystemRegistrySubsystem* Registry = GameInstance->GetSubsystem<UOmniSystemRegistrySubsystem>();
	if (!Registry || !Registry->IsRegistryInitialized())
	{
		return false;
	}

	UOmniStatusSystem* StatusSystem = Cast<UOmniStatusSystem>(Registry->GetSystemById(TEXT("Status")));
	if (!StatusSystem)
	{
		if (!bLoggedMissingStatusSystem)
		{
			bLoggedMissingStatusSystem = true;
			UE_LOG(
				LogOmniUIBridgeSubsystem,
				Warning,
				TEXT("[Omni][UI][HUD] StatusSystem indisponivel no Registry; HUD aguardando snapshot")
			);
		}
		return false;
	}

	bLoggedMissingStatusSystem = false;
	CachedSnapshot = StatusSystem->GetSnapshot();
	bHasCachedSnapshot = true;
	return true;
}

APlayerController* UOmniUIBridgeSubsystem::ResolveLocalPlayerController() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetFirstLocalPlayerController();
	}

	return nullptr;
}

UClass* UOmniUIBridgeSubsystem::ResolveHudWidgetClass(FString& OutPath, FString& OutFallbackReason)
{
	OutFallbackReason.Empty();
	OutPath = ConfiguredWidgetClassPath.IsEmpty() ? TEXT("<unset>") : ConfiguredWidgetClassPath;

	if (HudWidgetClass)
	{
		return HudWidgetClass.Get();
	}

	switch (HudWidgetConfigState)
	{
	case EHudWidgetConfigState::InvalidClass:
		OutFallbackReason = TEXT("InvalidClass");
		break;
	case EHudWidgetConfigState::Valid:
		OutFallbackReason = TEXT("MissingWBP");
		break;
	case EHudWidgetConfigState::Unknown:
	case EHudWidgetConfigState::MissingWBP:
	default:
		OutFallbackReason = TEXT("MissingWBP");
		break;
	}

	return UOmniHudWidget::StaticClass();
}

void UOmniUIBridgeSubsystem::LogFallbackActivated(const FString& Reason, const FString& Path)
{
	if (bLoggedFallbackActivated)
	{
		return;
	}

	bLoggedFallbackActivated = true;
	UE_LOG(
		LogOmniUIBridgeSubsystem,
		Warning,
		TEXT("[Omni][UI][HUD] Fallback activated | reason=%s path=%s"),
		Reason.IsEmpty() ? TEXT("Unknown") : *Reason,
		Path.IsEmpty() ? TEXT("<unset>") : *Path
	);
}
