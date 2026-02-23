#include "Library/OmniStatusLibrary.h"

UOmniDevStatusLibrary::UOmniDevStatusLibrary()
{
	if (Settings.ExhaustedTag.IsValid())
	{
		return;
	}

	const FGameplayTag ExhaustedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Exhausted"), false);
	if (ExhaustedTag.IsValid())
	{
		Settings.ExhaustedTag = ExhaustedTag;
	}
}
