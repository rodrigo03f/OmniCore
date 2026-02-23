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
		Entry.SetSetting(
			TEXT("StatusProfileAssetPath"),
			TEXT("/Game/Omni/Data/Status/DA_Omni_StatusProfile_Default.DA_Omni_StatusProfile_Default")
		);
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("ActionGate");
		Entry.SystemClass = UOmniActionGateSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("Status") };
		Entry.SetSetting(
			TEXT("ActionProfileAssetPath"),
			TEXT("/Game/Omni/Data/Action/DA_Omni_ActionProfile_Default.DA_Omni_ActionProfile_Default")
		);
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Movement");
		Entry.SystemClass = UOmniMovementSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("ActionGate"), TEXT("Status") };
		Entry.SetSetting(
			TEXT("MovementProfileAssetPath"),
			TEXT("/Game/Omni/Data/Movement/DA_Omni_MovementProfile_Default.DA_Omni_MovementProfile_Default")
		);
	}
}
