#include "Systems/OmniRuntimeSystem.h"

FName UOmniRuntimeSystem::GetSystemId_Implementation() const
{
	if (SystemId != NAME_None)
	{
		return SystemId;
	}

	return GetClass() ? GetClass()->GetFName() : NAME_None;
}

TArray<FName> UOmniRuntimeSystem::GetDependencies_Implementation() const
{
	return Dependencies;
}

void UOmniRuntimeSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	(void)WorldContextObject;
	(void)Manifest;
	bInitializationSuccessful = true;
}

void UOmniRuntimeSystem::ShutdownSystem_Implementation()
{
}

bool UOmniRuntimeSystem::IsTickEnabled_Implementation() const
{
	return false;
}

void UOmniRuntimeSystem::TickSystem_Implementation(float DeltaTime)
{
	(void)DeltaTime;
}

bool UOmniRuntimeSystem::HandleCommand_Implementation(const FOmniCommandMessage& Command)
{
	(void)Command;
	return false;
}

bool UOmniRuntimeSystem::HandleQuery_Implementation(FOmniQueryMessage& Query)
{
	(void)Query;
	return false;
}

void UOmniRuntimeSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	(void)Event;
}

bool UOmniRuntimeSystem::IsInitializationSuccessful() const
{
	return bInitializationSuccessful;
}

void UOmniRuntimeSystem::SetInitializationResult(const bool bSuccess)
{
	bInitializationSuccessful = bSuccess;
}
