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

