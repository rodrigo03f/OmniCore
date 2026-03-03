#include "Avatar/OmniAvatarBridgeComponent.h"

#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/UI/OmniUIBridgeSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniAvatarBridgeComponent, Log, All);

namespace OmniAvatarBridge
{
	static const FName MovementSystemId(TEXT("Movement"));
	static const FName ConfigSectionMovement(TEXT("Omni.Movement"));
	static const FName ConfigKeySprintMultiplier(TEXT("SprintMultiplier"));
	static const FName SprintRequestedTagName(TEXT("Game.Movement.Intent.SprintRequested"));
	static const FName CameraModeTpsTagName(TEXT("Game.Camera.Mode.TPS"));
}

UOmniAvatarBridgeComponent::UOmniAvatarBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UOmniAvatarBridgeComponent::BeginPlay()
{
	Super::BeginPlay();

	SprintRequestedTag = FGameplayTag::RequestGameplayTag(OmniAvatarBridge::SprintRequestedTagName, false);
	CameraModeTpsTag = FGameplayTag::RequestGameplayTag(OmniAvatarBridge::CameraModeTpsTagName, false);

	ResolveConfig();
	ResolveHost();
	ResolveRegistryAndSystems();
	RefreshContextTags();
	ApplyMovementBackend();
	EnsureHud();

	UE_LOG(
		LogOmniAvatarBridgeComponent,
		Log,
		TEXT("[Omni][AvatarBridge][Init] host=%s backend=%s baseSpeed=%.2f sprintMultiplier=%.2f"),
		HostPawn.IsValid() ? *GetNameSafe(HostPawn.Get()) : TEXT("<invalid>"),
		CharacterMovement.IsValid() ? TEXT("CMC") : TEXT("<none>"),
		BaseSpeed,
		SprintMultiplier
	);
}

void UOmniAvatarBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	(void)EndPlayReason;

	MovementSystem.Reset();
	Registry.Reset();
	UIBridgeSubsystem.Reset();
	CharacterMovement.Reset();
	HostPawn.Reset();
	ContextTags.Reset();

	Super::EndPlay(EndPlayReason);
}

void UOmniAvatarBridgeComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	(void)DeltaTime;

	ResolveHost();
	ResolveRegistryAndSystems();
	RefreshContextTags();
	ApplyMovementBackend();
	EnsureHud();
}

FGameplayTagContainer UOmniAvatarBridgeComponent::GetContextTags() const
{
	return ContextTags;
}

float UOmniAvatarBridgeComponent::GetBaseSpeed() const
{
	return BaseSpeed;
}

float UOmniAvatarBridgeComponent::GetEffectiveSpeed() const
{
	return EffectiveSpeed;
}

void UOmniAvatarBridgeComponent::ResolveHost()
{
	if (!HostPawn.IsValid())
	{
		HostPawn = Cast<APawn>(GetOwner());
		if (!HostPawn.IsValid())
		{
			if (!bLoggedInvalidHost)
			{
				bLoggedInvalidHost = true;
				UE_LOG(
					LogOmniAvatarBridgeComponent,
					Warning,
					TEXT("[Omni][AvatarBridge][Fallback] InvalidHost | owner=%s"),
					*GetNameSafe(GetOwner())
				);
			}
			return;
		}
	}

	bLoggedInvalidHost = false;

	if (!CharacterMovement.IsValid())
	{
		ACharacter* HostCharacter = Cast<ACharacter>(HostPawn.Get());
		if (HostCharacter)
		{
			CharacterMovement = HostCharacter->GetCharacterMovement();
		}
		else if (HostPawn.IsValid())
		{
			CharacterMovement = HostPawn->FindComponentByClass<UCharacterMovementComponent>();
		}

		if (!CharacterMovement.IsValid())
		{
			if (!bLoggedMissingCmc)
			{
				bLoggedMissingCmc = true;
				UE_LOG(
					LogOmniAvatarBridgeComponent,
					Warning,
					TEXT("[Omni][AvatarBridge][Fallback] MissingCMC | owner=%s"),
					*GetNameSafe(HostPawn.Get())
				);
			}
			return;
		}
	}

	bLoggedMissingCmc = false;

	if (!bHasCapturedBaseSpeed && CharacterMovement.IsValid())
	{
		BaseSpeed = FMath::Max(0.0f, CharacterMovement->MaxWalkSpeed);
		EffectiveSpeed = BaseSpeed;
		bHasCapturedBaseSpeed = BaseSpeed > 0.0f;
	}
}

void UOmniAvatarBridgeComponent::ResolveRegistryAndSystems()
{
	if (!Registry.IsValid())
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			Registry = GameInstance->GetSubsystem<UOmniSystemRegistrySubsystem>();
		}
	}

	if (!Registry.IsValid() || !Registry->IsRegistryInitialized())
	{
		if (!bLoggedMissingRegistry)
		{
			bLoggedMissingRegistry = true;
			UE_LOG(
				LogOmniAvatarBridgeComponent,
				Warning,
				TEXT("[Omni][AvatarBridge][Fallback] RegistryUnavailable | owner=%s"),
				*GetNameSafe(GetOwner())
			);
		}
		return;
	}

	bLoggedMissingRegistry = false;

	MovementSystem = Cast<UOmniMovementSystem>(Registry->GetSystemById(OmniAvatarBridge::MovementSystemId));
	if (!MovementSystem.IsValid())
	{
		if (!bLoggedMissingMovementSystem)
		{
			bLoggedMissingMovementSystem = true;
			UE_LOG(
				LogOmniAvatarBridgeComponent,
				Warning,
				TEXT("[Omni][AvatarBridge][Fallback] MovementSystemUnavailable | expected=%s"),
				*OmniAvatarBridge::MovementSystemId.ToString()
			);
		}
		return;
	}

	bLoggedMissingMovementSystem = false;
}

void UOmniAvatarBridgeComponent::ResolveConfig()
{
	float SprintMultiplierFromConfig = SprintMultiplier;
	if (GConfig->GetFloat(
		*OmniAvatarBridge::ConfigSectionMovement.ToString(),
		*OmniAvatarBridge::ConfigKeySprintMultiplier.ToString(),
		SprintMultiplierFromConfig,
		GGameIni
	))
	{
		SprintMultiplier = FMath::Max(1.0f, SprintMultiplierFromConfig);
	}
}

void UOmniAvatarBridgeComponent::RefreshContextTags()
{
	ContextTags.Reset();

	if (CameraModeTpsTag.IsValid())
	{
		ContextTags.AddTag(CameraModeTpsTag);
	}

	if (MovementSystem.IsValid() && MovementSystem->IsSprintRequested() && SprintRequestedTag.IsValid())
	{
		ContextTags.AddTag(SprintRequestedTag);
	}
}

void UOmniAvatarBridgeComponent::ApplyMovementBackend()
{
	if (!CharacterMovement.IsValid() || !bHasCapturedBaseSpeed)
	{
		return;
	}

	const bool bIsSprinting = MovementSystem.IsValid() && MovementSystem->IsSprinting();
	const float RuntimeMultiplier = bIsSprinting ? SprintMultiplier : 1.0f;
	const float NewEffectiveSpeed = BaseSpeed * RuntimeMultiplier;
	EffectiveSpeed = NewEffectiveSpeed;

	if (!FMath::IsNearlyEqual(CharacterMovement->MaxWalkSpeed, NewEffectiveSpeed))
	{
		CharacterMovement->MaxWalkSpeed = NewEffectiveSpeed;
	}
}

void UOmniAvatarBridgeComponent::EnsureHud()
{
	if (!bAutoEnsureHud || bLoggedHudReady)
	{
		return;
	}

	if (!UIBridgeSubsystem.IsValid())
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			UIBridgeSubsystem = GameInstance->GetSubsystem<UOmniUIBridgeSubsystem>();
		}
	}

	if (!UIBridgeSubsystem.IsValid())
	{
		if (!bLoggedHudFallback)
		{
			bLoggedHudFallback = true;
			UE_LOG(LogOmniAvatarBridgeComponent, Warning, TEXT("[Omni][AvatarBridge][Fallback] UIBridgeUnavailable"));
		}
		return;
	}

	if (!UIBridgeSubsystem->EnsureHudCreated())
	{
		if (!bLoggedHudFallback)
		{
			bLoggedHudFallback = true;
			UE_LOG(LogOmniAvatarBridgeComponent, Warning, TEXT("[Omni][AvatarBridge][Fallback] EnsureHudFailed"));
		}
		return;
	}

	bLoggedHudFallback = false;
	bLoggedHudReady = true;
	UE_LOG(LogOmniAvatarBridgeComponent, Log, TEXT("[Omni][AvatarBridge][HUD] Ready via UIBridgeSubsystem"));
}

