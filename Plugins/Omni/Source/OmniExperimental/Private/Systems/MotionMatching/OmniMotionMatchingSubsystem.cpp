#include "Systems/MotionMatching/OmniMotionMatchingSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Systems/Attributes/OmniAttributesSystem.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniMotionMatchingSubsystem, Log, All);

namespace OmniMotionMatching
{
	static const FName DefaultAnimSetTagName(TEXT("Game.Anim.Set.MM.Debug"));
	static const FName MovementSystemId(TEXT("Movement"));
	static const FName AttributesSystemId(TEXT("Attributes"));
}

void UOmniMotionMatchingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RuntimeState.bEnabled = false;
	RuntimeState.ActiveAnimSetTag = FGameplayTag::RequestGameplayTag(OmniMotionMatching::DefaultAnimSetTagName, false);
}

void UOmniMotionMatchingSubsystem::Deinitialize()
{
	RuntimeState = FOmniMMState{};

	Super::Deinitialize();
}

void UOmniMotionMatchingSubsystem::SetEnabled(const bool bInEnabled)
{
	RuntimeState.bEnabled = bInEnabled;
}

bool UOmniMotionMatchingSubsystem::IsEnabled() const
{
	return RuntimeState.bEnabled;
}

FString UOmniMotionMatchingSubsystem::BuildStatusLine() const
{
	float Speed = 0.0f;
	bool bIsSprinting = false;
	bool bIsExhausted = false;
	GatherRuntimeInputs(Speed, bIsSprinting, bIsExhausted);

	const FString Decision = EvaluatePlaceholderDecision(Speed, bIsSprinting, bIsExhausted);

	return FString::Printf(
		TEXT("[Omni][MM] enabled=%s tag=%s decision=%s speed=%.2f sprinting=%s exhausted=%s"),
		RuntimeState.bEnabled ? TEXT("true") : TEXT("false"),
		*GetActiveAnimSetTagString(),
		*Decision,
		Speed,
		bIsSprinting ? TEXT("true") : TEXT("false"),
		bIsExhausted ? TEXT("true") : TEXT("false")
	);
}

void UOmniMotionMatchingSubsystem::GatherRuntimeInputs(float& OutSpeed, bool& OutIsSprinting, bool& OutIsExhausted) const
{
	OutSpeed = 0.0f;
	OutIsSprinting = false;
	OutIsExhausted = false;

	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	if (const APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController())
	{
		if (const APawn* Pawn = PlayerController->GetPawn())
		{
			OutSpeed = Pawn->GetVelocity().Size2D();
		}
	}

	const UOmniSystemRegistrySubsystem* Registry = GameInstance->GetSubsystem<UOmniSystemRegistrySubsystem>();
	if (!Registry || !Registry->IsRegistryInitialized())
	{
		return;
	}

	const UOmniMovementSystem* MovementSystem = Cast<UOmniMovementSystem>(
		Registry->GetSystemById(OmniMotionMatching::MovementSystemId)
	);
	if (MovementSystem)
	{
		OutIsSprinting = MovementSystem->IsSprinting();
	}

	const UOmniAttributesSystem* AttributesSystem = Cast<UOmniAttributesSystem>(
		Registry->GetSystemById(OmniMotionMatching::AttributesSystemId)
	);
	if (AttributesSystem)
	{
		OutIsExhausted = AttributesSystem->IsExhausted();
	}
}

FString UOmniMotionMatchingSubsystem::EvaluatePlaceholderDecision(
	const float Speed,
	const bool bIsSprinting,
	const bool bIsExhausted
) const
{
	if (bIsExhausted)
	{
		return TEXT("Exhausted");
	}

	if (bIsSprinting)
	{
		return TEXT("Sprint");
	}

	if (Speed > KINDA_SMALL_NUMBER)
	{
		return TEXT("Locomotion");
	}

	return TEXT("Idle");
}

FString UOmniMotionMatchingSubsystem::GetActiveAnimSetTagString() const
{
	if (RuntimeState.ActiveAnimSetTag.IsValid())
	{
		return RuntimeState.ActiveAnimSetTag.ToString();
	}

	return OmniMotionMatching::DefaultAnimSetTagName.ToString();
}
