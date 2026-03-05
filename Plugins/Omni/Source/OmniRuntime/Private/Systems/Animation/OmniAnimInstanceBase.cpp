#include "Systems/Animation/OmniAnimInstanceBase.h"

namespace OmniAnimInstanceBase
{
	static const FName DefaultAnimSetTagName(TEXT("Game.Anim.Set.Default"));

	static FGameplayTag ResolveDefaultAnimSetTag()
	{
		return FGameplayTag::RequestGameplayTag(DefaultAnimSetTagName, false);
	}
}

UOmniAnimInstanceBase::UOmniAnimInstanceBase()
{
	ActiveAnimSetTag = OmniAnimInstanceBase::ResolveDefaultAnimSetTag();
}

void UOmniAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (!ActiveAnimSetTag.IsValid())
	{
		ActiveAnimSetTag = OmniAnimInstanceBase::ResolveDefaultAnimSetTag();
	}
}

void UOmniAnimInstanceBase::ApplyBridgeFrame(const FOmniAnimBridgeFrame& BridgeFrame)
{
	Speed = BridgeFrame.Speed;
	bIsSprinting = BridgeFrame.bIsSprinting;
	bIsExhausted = BridgeFrame.bIsExhausted;
	bIsCrouching = BridgeFrame.bIsCrouching;
	bIsInAir = BridgeFrame.bIsInAir;
	VerticalSpeed = BridgeFrame.VerticalSpeed;
	ActiveAnimSetTag = BridgeFrame.ActiveAnimSetTag.IsValid()
		? BridgeFrame.ActiveAnimSetTag
		: OmniAnimInstanceBase::ResolveDefaultAnimSetTag();
}
