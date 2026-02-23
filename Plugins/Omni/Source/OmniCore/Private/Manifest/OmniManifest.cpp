#include "Manifest/OmniManifest.h"

void FOmniSystemManifestEntry::SetSetting(const FName Key, const FString& Value)
{
	if (Key == NAME_None)
	{
		return;
	}

	Settings.Add(Key, Value);
}

bool FOmniSystemManifestEntry::TryGetSetting(const FName Key, FString& OutValue) const
{
	OutValue.Reset();

	if (Key == NAME_None)
	{
		return false;
	}

	if (const FString* Value = Settings.Find(Key))
	{
		OutValue = *Value;
		return true;
	}

	return false;
}

bool FOmniSystemManifestEntry::HasSetting(const FName Key) const
{
	if (Key == NAME_None)
	{
		return false;
	}

	return Settings.Contains(Key);
}

TArray<FName> FOmniSystemManifestEntry::GetSettingKeysSnapshot() const
{
	TArray<FName> Result;
	Settings.GenerateKeyArray(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

const FOmniSystemManifestEntry* UOmniManifest::FindEntryById(const FName SystemId) const
{
	if (SystemId == NAME_None)
	{
		return nullptr;
	}

	return Systems.FindByPredicate(
		[SystemId](const FOmniSystemManifestEntry& Entry)
		{
			return Entry.SystemId == SystemId;
		}
	);
}
