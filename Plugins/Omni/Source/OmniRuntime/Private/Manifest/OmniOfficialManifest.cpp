#include "Manifest/OmniOfficialManifest.h"

#include "Systems/ActionGate/OmniActionGateSystem.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/Status/OmniStatusSystem.h"

UOmniOfficialManifest::UOmniOfficialManifest()
{
	if (Systems.Num() > 0)
	{
		return;
	}

	Namespace = TEXT("Omni.Official");
	BuildVersion = 1;

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Status");
		Entry.SystemClass = UOmniStatusSystem::StaticClass();
		Entry.bEnabled = true;
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("ActionGate");
		Entry.SystemClass = UOmniActionGateSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("Status") };
		Entry.Settings.Add(TEXT("ActionProfileClassPath"), TEXT("/Script/OmniRuntime.OmniDevActionProfile"));
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Movement");
		Entry.SystemClass = UOmniMovementSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("ActionGate"), TEXT("Status") };
	}
}
