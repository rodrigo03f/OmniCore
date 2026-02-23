#include "Systems/Movement/OmniMovementSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Manifest/OmniManifest.h"
#include "Systems/OmniClockSubsystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniMovementSystem, Log, All);

namespace OmniMovement
{
	static const FName CategoryName(TEXT("Movement"));
	static const FName SourceName(TEXT("MovementSystem"));
	static const FName SystemId(TEXT("Movement"));
	static const FName ActionGateSystemId(TEXT("ActionGate"));
	static const FName StatusSystemId(TEXT("Status"));
	static const FName QueryCanStartAction(TEXT("CanStartAction"));
	static const FName QueryIsExhausted(TEXT("IsExhausted"));
	static const FName CommandStartAction(TEXT("StartAction"));
	static const FName CommandStopAction(TEXT("StopAction"));
	static const FName CommandSetSprinting(TEXT("SetSprinting"));

	static bool TryParseBoolString(const FString& Value, bool& OutValue)
	{
		if (Value.Equals(TEXT("True"), ESearchCase::IgnoreCase) || Value == TEXT("1"))
		{
			OutValue = true;
			return true;
		}
		if (Value.Equals(TEXT("False"), ESearchCase::IgnoreCase) || Value == TEXT("0"))
		{
			OutValue = false;
			return true;
		}
		return false;
	}
}

FName UOmniMovementSystem::GetSystemId_Implementation() const
{
	return TEXT("Movement");
}

TArray<FName> UOmniMovementSystem::GetDependencies_Implementation() const
{
	return { OmniMovement::ActionGateSystemId, OmniMovement::StatusSystemId };
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
			ClockSubsystem = GameInstance->GetSubsystem<UOmniClockSubsystem>();
		}
	}

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
	ClockSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniMovementSystem, Log, TEXT("Movement system shutdown."));
}

bool UOmniMovementSystem::IsTickEnabled_Implementation() const
{
	return true;
}

void UOmniMovementSystem::TickSystem_Implementation(const float DeltaTime)
{
	if (!Registry.IsValid())
	{
		return;
	}

	const double WorldTime = GetNowSeconds();

	if (bUseKeyboardShiftAsSprintRequest)
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

	if (bIsSprinting && QueryStatusIsExhausted())
	{
		StopSprinting(TEXT("Exhausted"));
	}

	PublishTelemetry();
}

void UOmniMovementSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);

	if (Event.SourceSystem == OmniMovement::StatusSystemId && Event.EventName == TEXT("Exhausted"))
	{
		if (bIsSprinting)
		{
			StopSprinting(TEXT("ExhaustedEvent"));
		}
	}
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

void UOmniMovementSystem::StartSprinting()
{
	FString DenyReason;
	if (!QueryCanStartSprint(&DenyReason))
	{
		NextStartAttemptWorldTime = GetNowSeconds() + FailedRetryIntervalSeconds;
		return;
	}

	if (!DispatchStartSprint())
	{
		NextStartAttemptWorldTime = GetNowSeconds() + FailedRetryIntervalSeconds;
		return;
	}

	bIsSprinting = true;
	NextStartAttemptWorldTime = 0.0f;
	DispatchStatusSprinting(true);

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->LogEvent(OmniMovement::CategoryName, TEXT("Sprint iniciada"), OmniMovement::SourceName);
	}
}

void UOmniMovementSystem::StopSprinting(const FName Reason)
{
	if (!bIsSprinting)
	{
		DispatchStatusSprinting(false);
		return;
	}

	DispatchStopSprint(Reason);
	bIsSprinting = false;
	DispatchStatusSprinting(false);

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

double UOmniMovementSystem::GetNowSeconds() const
{
	if (ClockSubsystem.IsValid())
	{
		return ClockSubsystem->GetSimTime();
	}
	if (Registry.IsValid() && Registry->GetWorld())
	{
		return Registry->GetWorld()->GetTimeSeconds();
	}
	return 0.0;
}

bool UOmniMovementSystem::QueryStatusIsExhausted() const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniQueryMessage Query;
	Query.SourceSystem = OmniMovement::SystemId;
	Query.TargetSystem = OmniMovement::StatusSystemId;
	Query.QueryName = OmniMovement::QueryIsExhausted;

	if (!Registry->ExecuteQuery(Query) || !Query.bSuccess)
	{
		return false;
	}

	bool bExhausted = false;
	if (!OmniMovement::TryParseBoolString(Query.Result, bExhausted))
	{
		return false;
	}

	return bExhausted;
}

void UOmniMovementSystem::DispatchStatusSprinting(const bool bSprinting) const
{
	if (!Registry.IsValid())
	{
		return;
	}

	FOmniCommandMessage Command;
	Command.SourceSystem = OmniMovement::SystemId;
	Command.TargetSystem = OmniMovement::StatusSystemId;
	Command.CommandName = OmniMovement::CommandSetSprinting;
	Command.Arguments.Add(TEXT("bSprinting"), bSprinting ? TEXT("True") : TEXT("False"));
	Registry->DispatchCommand(Command);
}

bool UOmniMovementSystem::QueryCanStartSprint(FString* OutReason) const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniQueryMessage Query;
	Query.SourceSystem = OmniMovement::SystemId;
	Query.TargetSystem = OmniMovement::ActionGateSystemId;
	Query.QueryName = OmniMovement::QueryCanStartAction;
	Query.Arguments.Add(TEXT("ActionId"), SprintActionId.ToString());

	const bool bHandled = Registry->ExecuteQuery(Query);
	if (!bHandled)
	{
		return false;
	}

	if (OutReason)
	{
		const FString* Reason = Query.Output.Find(TEXT("Reason"));
		*OutReason = Reason ? *Reason : Query.Result;
	}

	return Query.bSuccess;
}

bool UOmniMovementSystem::DispatchStartSprint() const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniCommandMessage Command;
	Command.SourceSystem = OmniMovement::SystemId;
	Command.TargetSystem = OmniMovement::ActionGateSystemId;
	Command.CommandName = OmniMovement::CommandStartAction;
	Command.Arguments.Add(TEXT("ActionId"), SprintActionId.ToString());
	return Registry->DispatchCommand(Command);
}

bool UOmniMovementSystem::DispatchStopSprint(const FName Reason) const
{
	if (!Registry.IsValid())
	{
		return false;
	}

	FOmniCommandMessage Command;
	Command.SourceSystem = OmniMovement::SystemId;
	Command.TargetSystem = OmniMovement::ActionGateSystemId;
	Command.CommandName = OmniMovement::CommandStopAction;
	Command.Arguments.Add(TEXT("ActionId"), SprintActionId.ToString());
	if (Reason != NAME_None)
	{
		Command.Arguments.Add(TEXT("Reason"), Reason.ToString());
	}
	return Registry->DispatchCommand(Command);
}
