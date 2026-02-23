#include "Profile/OmniActionProfile.h"

#include "Library/OmniActionLibrary.h"

void UOmniActionProfile::ResolveDefinitions(TArray<FOmniActionDefinition>& OutDefinitions) const
{
	OutDefinitions.Reset();

	if (const UOmniActionLibrary* LoadedLibrary = ActionLibrary.LoadSynchronous())
	{
		OutDefinitions = LoadedLibrary->Definitions;
	}

	for (const FOmniActionDefinition& OverrideDefinition : OverrideDefinitions)
	{
		if (OverrideDefinition.ActionId == NAME_None)
		{
			continue;
		}

		const int32 ExistingIndex = OutDefinitions.IndexOfByPredicate(
			[&OverrideDefinition](const FOmniActionDefinition& CurrentDefinition)
			{
				return CurrentDefinition.ActionId == OverrideDefinition.ActionId;
			}
		);

		if (ExistingIndex != INDEX_NONE)
		{
			OutDefinitions[ExistingIndex] = OverrideDefinition;
		}
		else
		{
			OutDefinitions.Add(OverrideDefinition);
		}
	}
}

UOmniDevActionProfile::UOmniDevActionProfile()
{
	if (OverrideDefinitions.Num() > 0)
	{
		return;
	}

	const UOmniDevActionLibrary* DevLibrary = GetDefault<UOmniDevActionLibrary>();
	if (DevLibrary)
	{
		OverrideDefinitions = DevLibrary->Definitions;
	}
}
