#include "Manifest/OmniManifest.h"

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

