#include "OmniForgeModule.h"

#include "Forge/OmniForgeRunner.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FOmniForgeModule, OmniForge)

namespace OmniForgeConsole
{
	static bool ParseBoolValue(const FString& Value, const bool DefaultValue)
	{
		if (Value.Equals(TEXT("1")) || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("on"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("yes"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		if (Value.Equals(TEXT("0")) || Value.Equals(TEXT("false"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("off"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("no"), ESearchCase::IgnoreCase))
		{
			return false;
		}
		return DefaultValue;
	}

	static void HandleForgeRunCommand(const TArray<FString>& Args, UWorld* World)
	{
		(void)World;

		FOmniForgeInput Input;

		for (int32 Index = 0; Index < Args.Num(); ++Index)
		{
			const FString Arg = Args[Index].TrimStartAndEnd();
			if (Arg.IsEmpty())
			{
				continue;
			}

			FString Key;
			FString Value;
			if (Arg.Split(TEXT("="), &Key, &Value))
			{
				Key = Key.TrimStartAndEnd();
				Value = Value.TrimStartAndEnd();

				if (Key.Equals(TEXT("root"), ESearchCase::IgnoreCase))
				{
					Input.GenerationRoot = Value;
					continue;
				}
				if (Key.Equals(TEXT("manifestAsset"), ESearchCase::IgnoreCase))
				{
					Input.ManifestAssetPath = FSoftObjectPath(Value);
					continue;
				}
				if (Key.Equals(TEXT("manifestClass"), ESearchCase::IgnoreCase))
				{
					Input.ManifestClassPath = FSoftClassPath(Value);
					continue;
				}
				if (Key.Equals(TEXT("requireContentAssets"), ESearchCase::IgnoreCase))
				{
					Input.bRequireContentAssets = ParseBoolValue(Value, Input.bRequireContentAssets);
					continue;
				}
			}
			else if (Index == 0)
			{
				Input.GenerationRoot = Arg;
			}
		}

		const FOmniForgeResult Result = UOmniForgeRunner::Run(Input);
		const FOmniForgeReport& Report = Result.Report;

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[OmniForge] %s | Errors=%d Warnings=%d | Report=%s | Resolved=%s"),
			Report.bPassed ? TEXT("PASS") : TEXT("FAIL"),
			Report.ErrorCount,
			Report.WarningCount,
			*Report.OutputReportPath,
			*Report.OutputResolvedManifestPath
		);
	}

	static FAutoConsoleCommandWithWorldAndArgs OmniForgeRunCommand(
		TEXT("omni.forge.run"),
		TEXT("Run OmniForge pipeline. Optional args: root=/Game/Data manifestAsset=/Game/... manifestClass=/Script/... requireContentAssets=0|1"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleForgeRunCommand)
	);
}

void FOmniForgeModule::StartupModule()
{
}

void FOmniForgeModule::ShutdownModule()
{
}
