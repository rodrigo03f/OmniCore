#include "Manifest/OmniOfficialManifest.h"

#include "Systems/ActionGate/OmniActionGateSystem.h"
#include "Systems/Attributes/OmniAttributesSystem.h"
#include "Systems/Camera/OmniCameraSystem.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/Modifiers/OmniModifiersSystem.h"
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
		Entry.SystemId = TEXT("Attributes");
		Entry.SystemClass = UOmniAttributesSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.SetSetting(
			TEXT("AttributesRecipeAssetPath"),
			TEXT("/Game/Data/Status/DA_AttributesRecipe_Default.DA_AttributesRecipe_Default")
		);
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Status");
		Entry.SystemClass = UOmniStatusSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("Attributes") };
		Entry.SetSetting(
			TEXT("StatusProfileAssetPath"),
			TEXT("/Game/Data/Status/DA_Omni_StatusProfile_Default.DA_Omni_StatusProfile_Default")
		);
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("ActionGate");
		Entry.SystemClass = UOmniActionGateSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("Status"), TEXT("Attributes") };
		Entry.SetSetting(
			TEXT("ActionProfileAssetPath"),
			TEXT("/Game/Data/Action/DA_Omni_ActionProfile_Default.DA_Omni_ActionProfile_Default")
		);
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Modifiers");
		Entry.SystemClass = UOmniModifiersSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("Attributes") };
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Movement");
		Entry.SystemClass = UOmniMovementSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("ActionGate"), TEXT("Attributes"), TEXT("Modifiers") };
		Entry.SetSetting(
			TEXT("MovementProfileAssetPath"),
			TEXT("/Game/Data/Movement/DA_Omni_MovementProfile_Default.DA_Omni_MovementProfile_Default")
		);
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Camera");
		Entry.SystemClass = UOmniCameraSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("Status"), TEXT("Attributes") };
	}
}
