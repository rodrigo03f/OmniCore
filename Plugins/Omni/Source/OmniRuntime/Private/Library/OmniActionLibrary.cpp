#include "Library/OmniActionLibrary.h"

UOmniDevActionLibrary::UOmniDevActionLibrary()
{
	if (Definitions.Num() > 0)
	{
		return;
	}

	FOmniActionDefinition SprintAction;
	SprintAction.ActionId = TEXT("Movement.Sprint");
	SprintAction.Policy = EOmniActionPolicy::DenyIfActive;

	const FGameplayTag ExhaustedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Exhausted"), false);
	if (ExhaustedTag.IsValid())
	{
		SprintAction.BlockedBy.AddTag(ExhaustedTag);
	}

	const FGameplayTag SprintLockTag = FGameplayTag::RequestGameplayTag(TEXT("Lock.Movement.Sprint"), false);
	if (SprintLockTag.IsValid())
	{
		SprintAction.AppliesLocks.AddTag(SprintLockTag);
	}

	Definitions.Add(MoveTemp(SprintAction));
}
