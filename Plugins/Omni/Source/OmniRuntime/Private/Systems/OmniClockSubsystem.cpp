#include "Systems/OmniClockSubsystem.h"

#include "Engine/World.h"

void UOmniClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetClock();
}

void UOmniClockSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UOmniClockSubsystem::Tick(const float DeltaTime)
{
	TickIndex++;

	if (bUseWorldTimeProvider)
	{
		if (const UWorld* World = GetWorld())
		{
			SimTimeSeconds = World->GetTimeSeconds();
			return;
		}
	}

	SimTimeSeconds += FMath::Max(0.0f, DeltaTime);
}

TStatId UOmniClockSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOmniClockSubsystem, STATGROUP_Tickables);
}

bool UOmniClockSubsystem::IsTickable() const
{
	return !IsTemplate();
}

UWorld* UOmniClockSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

double UOmniClockSubsystem::GetSimTime() const
{
	return SimTimeSeconds;
}

int64 UOmniClockSubsystem::GetTickIndex() const
{
	return TickIndex;
}

void UOmniClockSubsystem::ResetClock()
{
	SimTimeSeconds = 0.0;
	TickIndex = 0;
}
