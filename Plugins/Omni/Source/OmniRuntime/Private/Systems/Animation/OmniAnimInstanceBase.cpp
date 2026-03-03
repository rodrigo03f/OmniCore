#include "Systems/Animation/OmniAnimInstanceBase.h"

namespace OmniAnimInstanceBase
{
	static const FName DefaultAnimSetTagName(TEXT("Game.Anim.Set.Default"));
}

UOmniAnimInstanceBase::UOmniAnimInstanceBase()
{
	ActiveAnimSetTag = FGameplayTag::RequestGameplayTag(OmniAnimInstanceBase::DefaultAnimSetTagName, false);
}

void UOmniAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (!ActiveAnimSetTag.IsValid())
	{
		ActiveAnimSetTag = FGameplayTag::RequestGameplayTag(OmniAnimInstanceBase::DefaultAnimSetTagName, false);
	}
}
