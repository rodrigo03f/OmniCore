#include "OmniRuntimeModule.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayTagsManager.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/Status/OmniStatusSystem.h"

IMPLEMENT_MODULE(FOmniRuntimeModule, OmniRuntime)

namespace OmniRuntimeConsole
{
	namespace TagGuard
	{
		static const TCHAR* ReservedOmniPrefix = TEXT("Omni.");

		static const TSet<FName>& GetOfficialOmniGameplayTags()
		{
			// Namespace Omni.* e reservado ao framework. Esta lista deve conter apenas tags oficiais.
			static const TSet<FName> OfficialTags = {};
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
					LogTemp,
					Log,
					TEXT("[Omni][TagGuard] Validacao de GameplayTags concluida: Omni.* encontrados=%d, violacoes=0."),
					OmniTagsFound
				);
				return;
			}

			InvalidOmniTags.Sort();
			const FString InvalidList = FString::Join(InvalidOmniTags, TEXT(", "));
			UE_LOG(
				LogTemp,
				Fatal,
				TEXT("[Omni][TagGuard] Falha de inicializacao: GameplayTag(s) Omni.* nao oficial(is): %s. O namespace Omni.* e reservado ao framework. Use Game.* ou prefixo do projeto (ex.: MeuProjeto.*)."),
				*InvalidList
			);
		}
	}

	static TAutoConsoleVariable<int32> OmniDebugModeCVar(
		TEXT("omni.debug"),
		1,
		TEXT("Modo de debug do Omni. 0=Desligado, 1=Basico, 2=Completo."),
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

	static void HandleOmniDebugToggleCommand()
	{
		const int32 AffectedSubsystems = ForEachDebugSubsystem(
			[](UOmniDebugSubsystem* DebugSubsystem)
			{
				DebugSubsystem->ToggleOverlay();
			}
		);

		UE_LOG(LogTemp, Log, TEXT("[Omni] Overlay de debug alternado. Subsystems afetados: %d."), AffectedSubsystems);
	}

	static void HandleOmniDebugClearCommand()
	{
		const int32 AffectedSubsystems = ForEachDebugSubsystem(
			[](UOmniDebugSubsystem* DebugSubsystem)
			{
				DebugSubsystem->ClearEntries();
			}
		);

		UE_LOG(LogTemp, Log, TEXT("[Omni] Entradas de debug limpas. Subsystems afetados: %d."), AffectedSubsystems);
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
			UE_LOG(LogTemp, Log, TEXT("[Omni] Sprint solicitada. Systems afetados: %d"), AffectedSystems);
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
			UE_LOG(LogTemp, Log, TEXT("[Omni] Sprint cancelada. Systems afetados: %d"), AffectedSystems);
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
			UE_LOG(LogTemp, Log, TEXT("[Omni] Sprint alternada. Systems afetados: %d"), AffectedSystems);
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
				LogTemp,
				Log,
				TEXT("[Omni] AutoSprint iniciada por %.1fs. Systems afetados: %d"),
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
				UE_LOG(
					LogTemp,
					Log,
					TEXT("[Omni] Status[%d] Stamina=%.1f/%.1f Exhausted=%s"),
					StatusCount,
					StatusSystem->GetCurrentStamina(),
					StatusSystem->GetMaxStamina(),
					StatusSystem->IsExhausted() ? TEXT("True") : TEXT("False")
				);
			}
		);

		if (StatusCount == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Omni] Nenhum StatusSystem encontrado (registry inicializado?)."));
		}
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
}

void FOmniRuntimeModule::StartupModule()
{
	OmniRuntimeConsole::TagGuard::ValidateReservedOmniGameplayTags();
}

void FOmniRuntimeModule::ShutdownModule()
{
}
