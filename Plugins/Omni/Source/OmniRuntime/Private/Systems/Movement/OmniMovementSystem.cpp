#include "Systems/Movement/OmniMovementSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Manifest/OmniManifest.h"
#include "Systems/ActionGate/OmniActionGateSystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/Status/OmniStatusSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniMovementSystem, Log, All);

namespace OmniMovement
{
	static const FName CategoryName(TEXT("Movement"));
	static const FName SourceName(TEXT("MovementSystem"));
}

FName UOmniMovementSystem::GetSystemId_Implementation() const
{
	return TEXT("Movement");
}

TArray<FName> UOmniMovementSystem::GetDependencies_Implementation() const
{
	return { TEXT("ActionGate"), TEXT("Status") };
}

void UOmniMovementSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);

	Registry = Cast<UOmniSystemRegistrySubsystem>(WorldContextObject);
	if (Registry.IsValid())
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>();
		}
	}

	TryResolveDependencies();

	bSprintRequested = false;
	bIsSprinting = false;
	NextStartAttemptWorldTime = 0.0f;
	AutoSprintRemainingSeconds = 0.0f;
	PublishTelemetry();

	UE_LOG(LogOmniMovementSystem, Log, TEXT("Movement system initialized. Manifest=%s"), *GetNameSafe(Manifest));
}

void UOmniMovementSystem::ShutdownSystem_Implementation()
{
	StopSprinting(TEXT("Shutdown"));

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("Movement.SprintRequested"));
		DebugSubsystem->RemoveMetric(TEXT("Movement.IsSprinting"));
		DebugSubsystem->RemoveMetric(TEXT("Movement.AutoSprintRemaining"));
		DebugSubsystem->LogEvent(OmniMovement::CategoryName, TEXT("Movement finalizado"), OmniMovement::SourceName);
	}

	DebugSubsystem.Reset();
	ActionGateSystem.Reset();
	StatusSystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniMovementSystem, Log, TEXT("Movement system shutdown."));
}

bool UOmniMovementSystem::IsTickEnabled_Implementation() const
{
	return true;
}

void UOmniMovementSystem::TickSystem_Implementation(const float DeltaTime)
{
	if (!TryResolveDependencies())
	{
		return;
	}

	const UWorld* World = Registry.IsValid() ? Registry->GetWorld() : nullptr;
	const float WorldTime = World ? World->GetTimeSeconds() : 0.0f;

	if (bUseKeyboardShiftAsSprintRequest && Registry.IsValid())
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
			{
				const bool bShiftDown = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
				if (bShiftDown != bSprintRequested)
				{
					SetSprintRequested(bShiftDown);
				}
			}
		}
	}

	if (AutoSprintRemainingSeconds > 0.0f)
	{
		AutoSprintRemainingSeconds = FMath::Max(0.0f, AutoSprintRemainingSeconds - DeltaTime);
		if (AutoSprintRemainingSeconds <= KINDA_SMALL_NUMBER)
		{
			SetSprintRequested(false);
			if (DebugSubsystem.IsValid())
			{
				DebugSubsystem->LogEvent(OmniMovement::CategoryName, TEXT("AutoSprint finalizado"), OmniMovement::SourceName);
			}
		}
	}

	if (bSprintRequested && !bIsSprinting && WorldTime >= NextStartAttemptWorldTime)
	{
		StartSprinting();
	}

	if (!bSprintRequested && bIsSprinting)
	{
		StopSprinting(TEXT("Released"));
	}

	if (bIsSprinting && StatusSystem.IsValid() && StatusSystem->IsExhausted())
	{
		StopSprinting(TEXT("Exhausted"));
	}

	PublishTelemetry();
}

void UOmniMovementSystem::SetSprintRequested(const bool bRequested)
{
	if (bSprintRequested == bRequested)
	{
		return;
	}

	bSprintRequested = bRequested;
	if (!bSprintRequested)
	{
		AutoSprintRemainingSeconds = 0.0f;
	}

	PublishTelemetry();
}

void UOmniMovementSystem::ToggleSprintRequested()
{
	SetSprintRequested(!bSprintRequested);
}

void UOmniMovementSystem::StartAutoSprint(const float DurationSeconds)
{
	AutoSprintRemainingSeconds = FMath::Max(0.0f, DurationSeconds);
	if (AutoSprintRemainingSeconds > 0.0f)
	{
		SetSprintRequested(true);
	}
}

bool UOmniMovementSystem::IsSprintRequested() const
{
	return bSprintRequested;
}

bool UOmniMovementSystem::IsSprinting() const
{
	return bIsSprinting;
}

float UOmniMovementSystem::GetAutoSprintRemainingSeconds() const
{
	return AutoSprintRemainingSeconds;
}

bool UOmniMovementSystem::TryResolveDependencies()
{
	if (!Registry.IsValid())
	{
		return false;
	}

	if (!ActionGateSystem.IsValid())
	{
		ActionGateSystem = Cast<UOmniActionGateSystem>(Registry->GetSystemById(TEXT("ActionGate")));
	}

	if (!StatusSystem.IsValid())
	{
		StatusSystem = Cast<UOmniStatusSystem>(Registry->GetSystemById(TEXT("Status")));
	}

	return ActionGateSystem.IsValid() && StatusSystem.IsValid();
}

void UOmniMovementSystem::StartSprinting()
{
	if (!ActionGateSystem.IsValid())
	{
		return;
	}

	FOmniActionGateDecision Decision;
	if (!ActionGateSystem->TryStartAction(SprintActionId, Decision))
	{
		NextStartAttemptWorldTime = Registry.IsValid() && Registry->GetWorld()
			? Registry->GetWorld()->GetTimeSeconds() + FailedRetryIntervalSeconds
			: FailedRetryIntervalSeconds;
		return;
	}

	bIsSprinting = true;
	NextStartAttemptWorldTime = 0.0f;

	if (StatusSystem.IsValid())
	{
		StatusSystem->SetSprinting(true);
	}

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->LogEvent(OmniMovement::CategoryName, TEXT("Sprint iniciada"), OmniMovement::SourceName);
	}
}

void UOmniMovementSystem::StopSprinting(const FName Reason)
{
	if (!bIsSprinting)
	{
		if (StatusSystem.IsValid())
		{
			StatusSystem->SetSprinting(false);
		}
		return;
	}

	if (ActionGateSystem.IsValid())
	{
		ActionGateSystem->StopAction(SprintActionId, Reason);
	}

	bIsSprinting = false;

	if (StatusSystem.IsValid())
	{
		StatusSystem->SetSprinting(false);
	}

	if (DebugSubsystem.IsValid())
	{
		const FString ReasonText = Reason == NAME_None ? TEXT("Unknown") : Reason.ToString();
		DebugSubsystem->LogEvent(
			OmniMovement::CategoryName,
			FString::Printf(TEXT("Sprint parada (%s)"), *ReasonText),
			OmniMovement::SourceName
		);
	}
}

void UOmniMovementSystem::PublishTelemetry() const
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("Movement.SprintRequested"), bSprintRequested ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(TEXT("Movement.IsSprinting"), bIsSprinting ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(TEXT("Movement.AutoSprintRemaining"), FString::Printf(TEXT("%.1fs"), AutoSprintRemainingSeconds));
}
