#include "OmniExperimentalModule.h"

#include "Systems/MotionMatching/OmniMotionMatchingSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FOmniExperimentalModule, OmniExperimental)
DEFINE_LOG_CATEGORY_STATIC(LogOmniExperimentalModule, Log, All);

namespace OmniExperimentalConsole
{
	static bool ParseEnableArgument(const FString& Value, bool& OutEnabled)
	{
		if (
			Value.Equals(TEXT("1"))
			|| Value.Equals(TEXT("true"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("on"), ESearchCase::IgnoreCase)
		)
		{
			OutEnabled = true;
			return true;
		}

		if (
			Value.Equals(TEXT("0"))
			|| Value.Equals(TEXT("false"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("off"), ESearchCase::IgnoreCase)
		)
		{
			OutEnabled = false;
			return true;
		}

		return false;
	}

	static UOmniMotionMatchingSubsystem* ResolveMotionMatchingSubsystem(UWorld* PreferredWorld)
	{
		if (PreferredWorld)
		{
			if (UGameInstance* GameInstance = PreferredWorld->GetGameInstance())
			{
				return GameInstance->GetSubsystem<UOmniMotionMatchingSubsystem>();
			}
		}

		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UGameInstance* GameInstance = WorldContext.OwningGameInstance;
			if (!GameInstance)
			{
				continue;
			}

			return GameInstance->GetSubsystem<UOmniMotionMatchingSubsystem>();
		}

		return nullptr;
	}

	static void HandleOmniMmCommand(const TArray<FString>& Args, UWorld* World)
	{
		const FString Command = Args.Num() > 0 ? Args[0].ToLower() : TEXT("status");
		UOmniMotionMatchingSubsystem* MotionMatchingSubsystem = ResolveMotionMatchingSubsystem(World);
		if (!MotionMatchingSubsystem)
		{
			UE_LOG(
				LogOmniExperimentalModule,
				Warning,
				TEXT("[Omni][MM] SubsystemUnavailable | command=%s"),
				*Command
			);
			return;
		}

		if (Command == TEXT("enable"))
		{
			if (Args.Num() < 2)
			{
				UE_LOG(
					LogOmniExperimentalModule,
					Warning,
					TEXT("[Omni][MM] Usage: omni.mm enable 0|1")
				);
				return;
			}

			bool bEnabled = false;
			if (!ParseEnableArgument(Args[1], bEnabled))
			{
				UE_LOG(
					LogOmniExperimentalModule,
					Warning,
					TEXT("[Omni][MM] Invalid enable value '%s' | Usage: omni.mm enable 0|1"),
					*Args[1]
				);
				return;
			}

			MotionMatchingSubsystem->SetEnabled(bEnabled);
			UE_LOG(LogOmniExperimentalModule, Log, TEXT("%s"), *MotionMatchingSubsystem->BuildStatusLine());
			return;
		}

		if (Command == TEXT("status"))
		{
			UE_LOG(LogOmniExperimentalModule, Log, TEXT("%s"), *MotionMatchingSubsystem->BuildStatusLine());
			return;
		}

		UE_LOG(
			LogOmniExperimentalModule,
			Warning,
			TEXT("[Omni][MM] Invalid command '%s' | Usage: omni.mm enable 0|1 | omni.mm status"),
			*Command
		);
	}

	static FAutoConsoleCommandWithWorldAndArgs OmniMmCommand(
		TEXT("omni.mm"),
		TEXT("Motion Matching experimental commands. Usage: omni.mm enable 0|1 | omni.mm status"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOmniMmCommand)
	);
}

void FOmniExperimentalModule::StartupModule()
{
}

void FOmniExperimentalModule::ShutdownModule()
{
}
