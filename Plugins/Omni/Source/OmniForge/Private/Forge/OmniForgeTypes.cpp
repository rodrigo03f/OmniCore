#include "Forge/OmniForgeTypes.h"

void FOmniForgeNormalizedSystem::SetSetting(FName Key, const FString& Value)
{
	Settings.Add(Key, Value);
}

bool FOmniForgeNormalizedSystem::TryGetSetting(FName Key, FString& OutValue) const
{
	if (const FString* Found = Settings.Find(Key))
	{
		OutValue = *Found;
		return true;
	}

	return false;
}

bool FOmniForgeNormalizedSystem::TryGetSettingPtr(FName Key, const FString*& OutPtr) const
{
	OutPtr = Settings.Find(Key);
	return OutPtr != nullptr;
}

void FOmniForgeNormalizedSystem::GetSettingKeys(TArray<FName>& OutKeys) const
{
	Settings.GenerateKeyArray(OutKeys);
}
