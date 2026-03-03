#include "OmniRuntimeModule.h"

#include "Avatar/OmniAvatarBridgeComponent.h"
#include "Debug/OmniDebugSubsystem.h"
#include "Systems/Animation/OmniAnimBridgeComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayTagsManager.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "Systems/Camera/OmniCameraSystem.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/Status/OmniStatusSystem.h"

IMPLEMENT_MODULE(FOmniRuntimeModule, OmniRuntime)
DEFINE_LOG_CATEGORY_STATIC(LogOmniRuntime, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogOmniTagGuard, Log, All);

namespace OmniRuntimeConsole
{
	namespace TagGuard
	{
		// Purpose:
		// - Proteger o namespace reservado Omni.* no startup do runtime.
		// Inputs:
		// - Conjunto total de gameplay tags registradas no projeto.
		// - Lista oficial centralizada em GetOfficialOmniGameplayTags().
		// Outputs:
		// - Log de validacao quando tudo esta conforme.
		// - Fail-fast (Fatal) quando detecta Omni.* fora do conjunto oficial.
		// Determinism:
		// - Validacao ocorre antes do gameplay; sem efeitos variaveis de runtime.
		// Failure modes:
		// - Tag ilegal Omni.* aborta startup e orienta usar Game.* ou prefixo do projeto.
		static const TCHAR* ReservedOmniPrefix = TEXT("Omni.");

		static const TSet<FName>& GetOfficialOmniGameplayTags()
		{
			// Namespace Omni.* e reservado ao framework. Esta lista deve conter apenas tags oficiais.
			static const TSet<FName> OfficialTags = {
				FName(TEXT("Omni.Gate.Allow.Ok")),
				FName(TEXT("Omni.Gate.Deny.Locked"))
			};
			return OfficialTags;
		}

		static void ValidateReservedOmniGameplayTags()
		{
			FGameplayTagContainer AllTags;
			UGameplayTagsManager::Get().RequestAllGameplayTags(AllTags, true);

			const TSet<FName>& OfficialOmniTags = GetOfficialOmniGameplayTags();
			TArray<FString> InvalidOmniTags;
			InvalidOmniTags.Reserve(AllTags.Num());
			int32 OmniTagsFound = 0;

			for (const FGameplayTag& Tag : AllTags)
			{
				const FString TagString = Tag.ToString();
				if (!TagString.StartsWith(ReservedOmniPrefix, ESearchCase::CaseSensitive))
				{
					continue;
				}
				++OmniTagsFound;

				if (OfficialOmniTags.Contains(Tag.GetTagName()))
				{
					continue;
				}

				InvalidOmniTags.Add(TagString);
			}

			if (InvalidOmniTags.Num() == 0)
			{
				UE_LOG(
					LogOmniTagGuard,
					Log,
					TEXT("[Omni][TagGuard][Startup] Validacao concluida | Omni.* encontrados=%d, violacoes=0"),
					OmniTagsFound
				);
				return;
			}

			InvalidOmniTags.Sort();
			const FString InvalidList = FString::Join(InvalidOmniTags, TEXT(", "));
			UE_LOG(
				LogOmniTagGuard,
				Fatal,
				TEXT("[Omni][TagGuard][Startup] Illegal Omni.* tag(s) detectada(s): %s | Use Game.* ou prefixo do projeto (ex.: MeuProjeto.*)"),
				*InvalidList
			);
		}
	}

	static TAutoConsoleVariable<int32> OmniDebugModeCVar(
		TEXT("omni.debug"),
		0,
		TEXT("Modo de debug do Omni. 0=Desligado (default), 1=Basico, 2=Completo."),
		ECVF_Default
	);

	static int32 ForEachDebugSubsystem(const TFunctionRef<void(UOmniDebugSubsystem*)>& Function)
	{
		if (!GEngine)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = WorldContext.OwningGameInstance;
			if (!GameInstance)
			{
				continue;
			}

			if (UOmniDebugSubsystem* DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>())
			{
				Function(DebugSubsystem);
				++Count;
			}
		}

		return Count;
	}

	static int32 ForEachMovementSystem(const TFunctionRef<void(UOmniMovementSystem*)>& Function)
	{
		if (!GEngine)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = WorldContext.OwningGameInstance;
			if (!GameInstance)
			{
				continue;
			}

			UOmniSystemRegistrySubsystem* Registry = GameInstance->GetSubsystem<UOmniSystemRegistrySubsystem>();
			if (!Registry || !Registry->IsRegistryInitialized())
			{
				continue;
			}

			UOmniMovementSystem* MovementSystem = Cast<UOmniMovementSystem>(Registry->GetSystemById(TEXT("Movement")));
			if (!MovementSystem)
			{
				continue;
			}

			Function(MovementSystem);
			++Count;
		}

		return Count;
	}

	static int32 ForEachCameraSystem(const TFunctionRef<void(UOmniCameraSystem*)>& Function)
	{
		if (!GEngine)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = WorldContext.OwningGameInstance;
			if (!GameInstance)
			{
				continue;
			}

			UOmniSystemRegistrySubsystem* Registry = GameInstance->GetSubsystem<UOmniSystemRegistrySubsystem>();
			if (!Registry || !Registry->IsRegistryInitialized())
			{
				continue;
			}

			UOmniCameraSystem* CameraSystem = Cast<UOmniCameraSystem>(Registry->GetSystemById(TEXT("Camera")));
			if (!CameraSystem)
			{
				continue;
			}

			Function(CameraSystem);
			++Count;
		}

		return Count;
	}

	static int32 ForEachStatusSystem(const TFunctionRef<void(UOmniStatusSystem*)>& Function)
	{
		if (!GEngine)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = WorldContext.OwningGameInstance;
			if (!GameInstance)
			{
				continue;
			}

			UOmniSystemRegistrySubsystem* Registry = GameInstance->GetSubsystem<UOmniSystemRegistrySubsystem>();
			if (!Registry || !Registry->IsRegistryInitialized())
			{
				continue;
			}

			UOmniStatusSystem* StatusSystem = Cast<UOmniStatusSystem>(Registry->GetSystemById(TEXT("Status")));
			if (!StatusSystem)
			{
				continue;
			}

			Function(StatusSystem);
			++Count;
		}

		return Count;
	}

	static int32 ForEachLocalPawn(const TFunctionRef<void(APawn*)>& Function)
	{
		if (!GEngine)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = WorldContext.OwningGameInstance;
			if (!GameInstance)
			{
				continue;
			}

			APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController();
			if (!PlayerController)
			{
				continue;
			}

			APawn* Pawn = PlayerController->GetPawn();
			if (!Pawn)
			{
				continue;
			}

			Function(Pawn);
			++Count;
		}

		return Count;
	}

	static void HandleOmniDebugToggleCommand()
	{
		const int32 AffectedSubsystems = ForEachDebugSubsystem(
			[](UOmniDebugSubsystem* DebugSubsystem)
			{
				DebugSubsystem->ToggleOverlay();
			}
		);

		UE_LOG(LogOmniRuntime, Log, TEXT("[Omni][Runtime][Console] Overlay de debug alternado | Subsystems afetados=%d"), AffectedSubsystems);
	}

	static void HandleOmniDebugClearCommand()
	{
		const int32 AffectedSubsystems = ForEachDebugSubsystem(
			[](UOmniDebugSubsystem* DebugSubsystem)
			{
				DebugSubsystem->ClearEntries();
			}
		);

		UE_LOG(LogOmniRuntime, Log, TEXT("[Omni][Runtime][Console] Entradas de debug limpas | Subsystems afetados=%d"), AffectedSubsystems);
	}

	static void HandleOmniSprintCommand(const TArray<FString>& Args, UWorld* World)
	{
		(void)World;

		const FString Command = Args.Num() > 0 ? Args[0].ToLower() : TEXT("status");

		if (Command == TEXT("start"))
		{
			const int32 AffectedSystems = ForEachMovementSystem(
				[](UOmniMovementSystem* MovementSystem)
				{
					MovementSystem->SetSprintRequested(true);
				}
			);
			UE_LOG(LogOmniRuntime, Log, TEXT("[Omni][Runtime][Console] Sprint solicitada | Systems afetados=%d"), AffectedSystems);
			return;
		}

		if (Command == TEXT("stop"))
		{
			const int32 AffectedSystems = ForEachMovementSystem(
				[](UOmniMovementSystem* MovementSystem)
				{
					MovementSystem->SetSprintRequested(false);
				}
			);
			UE_LOG(LogOmniRuntime, Log, TEXT("[Omni][Runtime][Console] Sprint cancelada | Systems afetados=%d"), AffectedSystems);
			return;
		}

		if (Command == TEXT("toggle"))
		{
			const int32 AffectedSystems = ForEachMovementSystem(
				[](UOmniMovementSystem* MovementSystem)
				{
					MovementSystem->ToggleSprintRequested();
				}
			);
			UE_LOG(LogOmniRuntime, Log, TEXT("[Omni][Runtime][Console] Sprint alternada | Systems afetados=%d"), AffectedSystems);
			return;
		}

		if (Command == TEXT("auto"))
		{
			float DurationSeconds = 10.0f;
			if (Args.Num() > 1)
			{
				LexFromString(DurationSeconds, *Args[1]);
			}
			DurationSeconds = FMath::Max(0.1f, DurationSeconds);

			const int32 AffectedSystems = ForEachMovementSystem(
				[DurationSeconds](UOmniMovementSystem* MovementSystem)
				{
					MovementSystem->StartAutoSprint(DurationSeconds);
				}
			);
			UE_LOG(
				LogOmniRuntime,
				Log,
				TEXT("[Omni][Runtime][Console] AutoSprint iniciada | duracao=%.1fs systemsAfetados=%d"),
				DurationSeconds,
				AffectedSystems
			);
			return;
		}

		int32 StatusCount = 0;
		ForEachStatusSystem(
			[&StatusCount](UOmniStatusSystem* StatusSystem)
			{
				++StatusCount;
				const FOmniAttributeSnapshot Snapshot = StatusSystem->GetSnapshot();
				UE_LOG(
					LogOmniRuntime,
					Log,
					TEXT("[Omni][Runtime][Console] Attributes[%d] hp=%.1f/%.1f stamina=%.1f/%.1f exhausted=%s"),
					StatusCount,
					Snapshot.HP.Current,
					Snapshot.HP.Max,
					Snapshot.Stamina.Current,
					Snapshot.Stamina.Max,
					Snapshot.bExhausted ? TEXT("True") : TEXT("False")
				);
			}
		);

		if (StatusCount == 0)
		{
			UE_LOG(
				LogOmniRuntime,
				Warning,
				TEXT("[Omni][Runtime][Console] Nenhum StatusSystem encontrado | Verifique Registry inicializado e Manifest configurado")
			);
		}
	}

	static void HandleOmniCameraCommand(const TArray<FString>& Args, UWorld* World)
	{
		(void)World;

		const FString Command = Args.Num() > 0 ? Args[0].ToLower() : TEXT("status");
		if (Command != TEXT("status"))
		{
			UE_LOG(
				LogOmniRuntime,
				Warning,
				TEXT("[Omni][Runtime][Console] Comando invalido para omni.camera: '%s' | Uso: omni.camera status"),
				*Command
			);
			return;
		}

		int32 CameraCount = 0;
		ForEachCameraSystem(
			[&CameraCount](UOmniCameraSystem* CameraSystem)
			{
				++CameraCount;
				const FOmniCameraSnapshot Snapshot = CameraSystem->GetSnapshot();
				UE_LOG(
					LogOmniRuntime,
					Log,
					TEXT("[Omni][Runtime][Console] Camera[%d] mode=%s rig=%s fov=%.2f ortho=%.2f arm=%.2f offset=%s fallback=%s"),
					CameraCount,
					Snapshot.ActiveModeTag.IsValid() ? *Snapshot.ActiveModeTag.ToString() : TEXT("<none>"),
					Snapshot.ActiveRigName == NAME_None ? TEXT("<none>") : *Snapshot.ActiveRigName.ToString(),
					Snapshot.CurrentFOV,
					Snapshot.CurrentOrthoWidth,
					Snapshot.CurrentArmLength,
					*Snapshot.CurrentOffset.ToCompactString(),
					Snapshot.bUsingFallback ? TEXT("True") : TEXT("False")
				);
			}
		);

		if (CameraCount == 0)
		{
			UE_LOG(
				LogOmniRuntime,
				Warning,
				TEXT("[Omni][Runtime][Console] Nenhum CameraSystem encontrado | Verifique Registry inicializado e Manifest configurado")
			);
		}
	}

	static void HandleOmniDamageCommand(const TArray<FString>& Args, UWorld* World)
	{
		(void)World;

		float Amount = 10.0f;
		if (Args.Num() > 0)
		{
			LexFromString(Amount, *Args[0]);
		}
		Amount = FMath::Max(0.0f, Amount);

		const int32 AffectedSystems = ForEachStatusSystem(
			[Amount](UOmniStatusSystem* StatusSystem)
			{
				StatusSystem->ApplyDamage(Amount);
			}
		);
		UE_LOG(
			LogOmniRuntime,
			Log,
			TEXT("[Omni][Runtime][Console] omni.damage %.1f | systemsAfetados=%d"),
			Amount,
			AffectedSystems
		);
	}

	static void HandleOmniHealCommand(const TArray<FString>& Args, UWorld* World)
	{
		(void)World;

		float Amount = 10.0f;
		if (Args.Num() > 0)
		{
			LexFromString(Amount, *Args[0]);
		}
		Amount = FMath::Max(0.0f, Amount);

		const int32 AffectedSystems = ForEachStatusSystem(
			[Amount](UOmniStatusSystem* StatusSystem)
			{
				StatusSystem->ApplyHeal(Amount);
			}
		);
		UE_LOG(
			LogOmniRuntime,
			Log,
			TEXT("[Omni][Runtime][Console] omni.heal %.1f | systemsAfetados=%d"),
			Amount,
			AffectedSystems
		);
	}

	static void HandleOmniAvatarBridgeCommand(const TArray<FString>& Args, UWorld* World)
	{
		(void)World;

		const FString Command = Args.Num() > 0 ? Args[0].ToLower() : TEXT("status");
		if (Command == TEXT("attach"))
		{
			int32 AttachedCount = 0;
			const int32 LocalPawns = ForEachLocalPawn(
				[&AttachedCount](APawn* Pawn)
				{
					if (!Pawn)
					{
						return;
					}

					UOmniAvatarBridgeComponent* ExistingBridge = Pawn->FindComponentByClass<UOmniAvatarBridgeComponent>();
					if (ExistingBridge)
					{
						++AttachedCount;
						return;
					}

					UOmniAvatarBridgeComponent* NewBridge = NewObject<UOmniAvatarBridgeComponent>(
						Pawn,
						UOmniAvatarBridgeComponent::StaticClass(),
						TEXT("OmniAvatarBridge")
					);
					if (!NewBridge)
					{
						return;
					}

					Pawn->AddInstanceComponent(NewBridge);
					NewBridge->RegisterComponent();
					++AttachedCount;
				}
			);

			UE_LOG(
				LogOmniRuntime,
				Log,
				TEXT("[Omni][Runtime][Console] omni.avatarbridge attach | localPawns=%d bridgesReady=%d"),
				LocalPawns,
				AttachedCount
			);
			return;
		}

		if (Command != TEXT("status"))
		{
			UE_LOG(
				LogOmniRuntime,
				Warning,
				TEXT("[Omni][Runtime][Console] Comando invalido para omni.avatarbridge: '%s' | Uso: omni.avatarbridge attach|status"),
				*Command
			);
			return;
		}

		int32 ReadyCount = 0;
		const int32 LocalPawns = ForEachLocalPawn(
			[&ReadyCount](APawn* Pawn)
			{
				if (Pawn && Pawn->FindComponentByClass<UOmniAvatarBridgeComponent>())
				{
					++ReadyCount;
				}
			}
		);

		UE_LOG(
			LogOmniRuntime,
			Log,
			TEXT("[Omni][Runtime][Console] omni.avatarbridge status | localPawns=%d bridgesReady=%d"),
			LocalPawns,
			ReadyCount
		);
	}

	static void HandleOmniAnimBridgeCommand(const TArray<FString>& Args, UWorld* World)
	{
		(void)World;

		const FString Command = Args.Num() > 0 ? Args[0].ToLower() : TEXT("status");
		if (Command == TEXT("attach"))
		{
			int32 AttachedCount = 0;
			const int32 LocalPawns = ForEachLocalPawn(
				[&AttachedCount](APawn* Pawn)
				{
					if (!Pawn)
					{
						return;
					}

					UOmniAnimBridgeComponent* ExistingBridge = Pawn->FindComponentByClass<UOmniAnimBridgeComponent>();
					if (ExistingBridge)
					{
						++AttachedCount;
						return;
					}

					UOmniAnimBridgeComponent* NewBridge = NewObject<UOmniAnimBridgeComponent>(
						Pawn,
						UOmniAnimBridgeComponent::StaticClass(),
						TEXT("OmniAnimBridge")
					);
					if (!NewBridge)
					{
						return;
					}

					Pawn->AddInstanceComponent(NewBridge);
					NewBridge->RegisterComponent();
					++AttachedCount;
				}
			);

			UE_LOG(
				LogOmniRuntime,
				Log,
				TEXT("[Omni][Runtime][Console] omni.animbridge attach | localPawns=%d bridgesReady=%d"),
				LocalPawns,
				AttachedCount
			);
			return;
		}

		if (Command != TEXT("status"))
		{
			UE_LOG(
				LogOmniRuntime,
				Warning,
				TEXT("[Omni][Runtime][Console] Comando invalido para omni.animbridge: '%s' | Uso: omni.animbridge attach|status"),
				*Command
			);
			return;
		}

		int32 ReadyCount = 0;
		const int32 LocalPawns = ForEachLocalPawn(
			[&ReadyCount](APawn* Pawn)
			{
				UOmniAnimBridgeComponent* Bridge = Pawn ? Pawn->FindComponentByClass<UOmniAnimBridgeComponent>() : nullptr;
				if (!Bridge)
				{
					return;
				}

				if (Bridge->IsBridgeReady())
				{
					++ReadyCount;
				}

				UE_LOG(
					LogOmniRuntime,
					Log,
					TEXT("[Omni][Runtime][Console] omni.animbridge status | owner=%s ready=%s speed=%.2f sprinting=%s exhausted=%s"),
					*GetNameSafe(Pawn),
					Bridge->IsBridgeReady() ? TEXT("True") : TEXT("False"),
					Bridge->GetDebugSpeed(),
					Bridge->GetDebugIsSprinting() ? TEXT("True") : TEXT("False"),
					Bridge->GetDebugIsExhausted() ? TEXT("True") : TEXT("False")
				);
			}
		);

		UE_LOG(
			LogOmniRuntime,
			Log,
			TEXT("[Omni][Runtime][Console] omni.animbridge status summary | localPawns=%d bridgesReady=%d"),
			LocalPawns,
			ReadyCount
		);
	}

	static FAutoConsoleCommand OmniDebugToggleCommand(
		TEXT("omni.debug.toggle"),
		TEXT("Alterna o overlay de debug do Omni."),
		FConsoleCommandDelegate::CreateStatic(&HandleOmniDebugToggleCommand)
	);

	static FAutoConsoleCommand OmniDebugClearCommand(
		TEXT("omni.debug.clear"),
		TEXT("Limpa as entradas de debug do Omni."),
		FConsoleCommandDelegate::CreateStatic(&HandleOmniDebugClearCommand)
	);

	static FAutoConsoleCommandWithWorldAndArgs OmniSprintCommand(
		TEXT("omni.sprint"),
		TEXT("Controle de sprint do Omni. Uso: omni.sprint start|stop|toggle|auto [segundos]|status"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOmniSprintCommand)
	);

	static FAutoConsoleCommandWithWorldAndArgs OmniCameraCommand(
		TEXT("omni.camera"),
		TEXT("Status da camera Omni. Uso: omni.camera status"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOmniCameraCommand)
	);

	static FAutoConsoleCommandWithWorldAndArgs OmniDamageCommand(
		TEXT("omni.damage"),
		TEXT("Aplica dano em Game.Attr.HP. Uso: omni.damage [valor]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOmniDamageCommand)
	);

	static FAutoConsoleCommandWithWorldAndArgs OmniHealCommand(
		TEXT("omni.heal"),
		TEXT("Aplica cura em Game.Attr.HP. Uso: omni.heal [valor]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOmniHealCommand)
	);

	static FAutoConsoleCommandWithWorldAndArgs OmniAvatarBridgeCommand(
		TEXT("omni.avatarbridge"),
		TEXT("Bridge de avatar Omni. Uso: omni.avatarbridge attach|status"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOmniAvatarBridgeCommand)
	);

	static FAutoConsoleCommandWithWorldAndArgs OmniAnimBridgeCommand(
		TEXT("omni.animbridge"),
		TEXT("Bridge de animacao Omni. Uso: omni.animbridge attach|status"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOmniAnimBridgeCommand)
	);
}

void FOmniRuntimeModule::StartupModule()
{
	OmniRuntimeConsole::TagGuard::ValidateReservedOmniGameplayTags();
}

void FOmniRuntimeModule::ShutdownModule()
{
}
