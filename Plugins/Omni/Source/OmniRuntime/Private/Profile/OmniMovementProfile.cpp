#include "Profile/OmniMovementProfile.h"

#include "Library/OmniMovementLibrary.h"

bool UOmniMovementProfile::ResolveSettings(FOmniMovementSettings& OutSettings) const
{
	OutSettings = FOmniMovementSettings();
	bool bHasAnySource = false;

	if (const UOmniMovementLibrary* LoadedLibrary = MovementLibrary.LoadSynchronous())
	{
		OutSettings = LoadedLibrary->Settings;
		bHasAnySource = true;
	}

	if (bUseOverrides)
	{
		OutSettings = OverrideSettings;
		bHasAnySource = true;
	}

	if (!bHasAnySource)
	{
		return false;
	}

	FString ValidationError;
	return OutSettings.IsValid(ValidationError);
}

UOmniDevMovementProfile::UOmniDevMovementProfile()
{
	if (bUseOverrides)
	{
		return;
	}

	const UOmniDevMovementLibrary* DevLibrary = GetDefault<UOmniDevMovementLibrary>();
	if (DevLibrary)
	{
		bUseOverrides = true;
		OverrideSettings = DevLibrary->Settings;
	}
}
