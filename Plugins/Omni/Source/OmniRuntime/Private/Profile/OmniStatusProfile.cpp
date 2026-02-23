#include "Profile/OmniStatusProfile.h"

#include "Library/OmniStatusLibrary.h"

bool UOmniStatusProfile::ResolveSettings(FOmniStatusSettings& OutSettings) const
{
	OutSettings = FOmniStatusSettings();
	bool bHasAnySource = false;

	if (const UOmniStatusLibrary* LoadedLibrary = StatusLibrary.LoadSynchronous())
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

UOmniDevStatusProfile::UOmniDevStatusProfile()
{
	if (bUseOverrides)
	{
		return;
	}

	const UOmniDevStatusLibrary* DevLibrary = GetDefault<UOmniDevStatusLibrary>();
	if (DevLibrary)
	{
		bUseOverrides = true;
		OverrideSettings = DevLibrary->Settings;
	}
}
