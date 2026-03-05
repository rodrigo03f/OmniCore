#include "Systems/Animation/OmniAnimBridgeComponent.h"

#include "Systems/Animation/OmniAnimInstanceBase.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/Status/OmniStatusSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniAnimBridgeComponent, Log, All);

namespace OmniAnimBridge
{
	static const FName MovementSystemId(TEXT("Movement"));
	static const FName StatusSystemId(TEXT("Status"));
	static const FName DefaultAnimSetTagName(TEXT("Game.Anim.Set.Default"));
}

UOmniAnimBridgeComponent::UOmniAnimBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UOmniAnimBridgeComponent::BeginPlay()
{
	Super::BeginPlay();

	DefaultAnimSetTag = FGameplayTag::RequestGameplayTag(OmniAnimBridge::DefaultAnimSetTagName, false);

	ResolveHost();
	ResolveRegistryAndSystems();
	ResolveAnimationTargets();
	ApplyOmniState();
}

void UOmniAnimBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	(void)EndPlayReason;

	StatusSystem.Reset();
	MovementSystem.Reset();
	Registry.Reset();
	OmniAnimInstance.Reset();
	SkeletalMeshComponent.Reset();
	CharacterMovement.Reset();
	HostPawn.Reset();

	Super::EndPlay(EndPlayReason);
}

void UOmniAnimBridgeComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	(void)DeltaTime;
	(void)TickType;
	(void)ThisTickFunction;

	ResolveHost();
	ResolveRegistryAndSystems();
	ResolveAnimationTargets();
	ApplyOmniState();
}

bool UOmniAnimBridgeComponent::IsBridgeReady() const
{
	return HostPawn.IsValid() && SkeletalMeshComponent.IsValid() && OmniAnimInstance.IsValid();
}

bool UOmniAnimBridgeComponent::IsOmniAnimInstanceResolved() const
{
	return OmniAnimInstance.IsValid();
}

float UOmniAnimBridgeComponent::GetDebugSpeed() const
{
	return DebugSpeed;
}

bool UOmniAnimBridgeComponent::GetDebugIsSprinting() const
{
	return bDebugIsSprinting;
}

bool UOmniAnimBridgeComponent::GetDebugIsExhausted() const
{
	return bDebugIsExhausted;
}

bool UOmniAnimBridgeComponent::IsCharacterMovementResolved() const
{
	return CharacterMovement.IsValid();
}

void UOmniAnimBridgeComponent::ResolveHost()
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
					LogOmniAnimBridgeComponent,
					Warning,
					TEXT("[Omni][AnimBridge][Fallback] InvalidHost | owner=%s"),
					*GetNameSafe(GetOwner())
				);
			}
			return;
		}
	}

	bLoggedInvalidHost = false;

	if (!CharacterMovement.IsValid())
	{
		if (ACharacter* Character = Cast<ACharacter>(HostPawn.Get()))
		{
			CharacterMovement = Character->GetCharacterMovement();
		}
		else
		{
			CharacterMovement = HostPawn->FindComponentByClass<UCharacterMovementComponent>();
		}
	}
}

void UOmniAnimBridgeComponent::ResolveRegistryAndSystems()
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
				LogOmniAnimBridgeComponent,
				Warning,
				TEXT("[Omni][AnimBridge][Fallback] RegistryUnavailable | owner=%s"),
				*GetNameSafe(GetOwner())
			);
		}
		return;
	}

	bLoggedMissingRegistry = false;

	if (!MovementSystem.IsValid())
	{
		MovementSystem = Cast<UOmniMovementSystem>(Registry->GetSystemById(OmniAnimBridge::MovementSystemId));
	}

	if (!StatusSystem.IsValid())
	{
		StatusSystem = Cast<UOmniStatusSystem>(Registry->GetSystemById(OmniAnimBridge::StatusSystemId));
	}
}

void UOmniAnimBridgeComponent::ResolveAnimationTargets()
{
	if (!HostPawn.IsValid())
	{
		return;
	}

	USkeletalMeshComponent* ResolvedMesh = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(HostPawn.Get()))
	{
		ResolvedMesh = Character->GetMesh();
	}
	if (!ResolvedMesh)
	{
		ResolvedMesh = HostPawn->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (SkeletalMeshComponent.Get() != ResolvedMesh)
	{
		USkeletalMeshComponent* PreviousMesh = SkeletalMeshComponent.Get();
		SkeletalMeshComponent = ResolvedMesh;
		OmniAnimInstance.Reset();
		bLoggedReady = false;
		if (PreviousMesh != nullptr && SkeletalMeshComponent.IsValid())
		{
			UE_LOG(
				LogOmniAnimBridgeComponent,
				Log,
				TEXT("[Omni][AnimBridge][Rebind] MeshChanged | owner=%s previousMesh=%s newMesh=%s"),
				*GetNameSafe(HostPawn.Get()),
				*GetNameSafe(PreviousMesh),
				*GetNameSafe(SkeletalMeshComponent.Get())
			);
		}
	}

	if (!SkeletalMeshComponent.IsValid())
	{
		if (!bLoggedMissingMesh)
		{
			bLoggedMissingMesh = true;
			UE_LOG(
				LogOmniAnimBridgeComponent,
				Warning,
				TEXT("[Omni][AnimBridge][Fallback] MissingSkeletalMesh | owner=%s"),
				*GetNameSafe(HostPawn.Get())
			);
		}
		return;
	}

	bLoggedMissingMesh = false;

	UAnimInstance* CurrentAnimInstance = SkeletalMeshComponent->GetAnimInstance();
	UOmniAnimInstanceBase* CurrentOmniAnimInstance = Cast<UOmniAnimInstanceBase>(CurrentAnimInstance);
	if (!CurrentOmniAnimInstance)
	{
		if (!bLoggedInvalidAnimInstance)
		{
			bLoggedInvalidAnimInstance = true;
			UE_LOG(
				LogOmniAnimBridgeComponent,
				Warning,
				TEXT("[Omni][AnimBridge][Fallback] InvalidAnimInstance | owner=%s animClass=%s expected=%s"),
				*GetNameSafe(HostPawn.Get()),
				*GetNameSafe(CurrentAnimInstance ? CurrentAnimInstance->GetClass() : nullptr),
				*UOmniAnimInstanceBase::StaticClass()->GetName()
			);
		}
		return;
	}

	if (OmniAnimInstance.Get() != CurrentOmniAnimInstance)
	{
		UOmniAnimInstanceBase* PreviousAnimInstance = OmniAnimInstance.Get();
		OmniAnimInstance = CurrentOmniAnimInstance;
		bLoggedReady = false;
		if (PreviousAnimInstance != nullptr)
		{
			UE_LOG(
				LogOmniAnimBridgeComponent,
				Log,
				TEXT("[Omni][AnimBridge][Rebind] AnimInstanceChanged | owner=%s previous=%s current=%s"),
				*GetNameSafe(HostPawn.Get()),
				*GetNameSafe(PreviousAnimInstance),
				*GetNameSafe(OmniAnimInstance.Get())
			);
		}
	}

	bLoggedInvalidAnimInstance = false;

	if (!bLoggedReady)
	{
		bLoggedReady = true;
		UE_LOG(
			LogOmniAnimBridgeComponent,
			Log,
			TEXT("[Omni][AnimBridge][Init] owner=%s mesh=%s animClass=%s"),
			*GetNameSafe(HostPawn.Get()),
			*GetNameSafe(SkeletalMeshComponent.Get()),
			*GetNameSafe(OmniAnimInstance->GetClass())
		);
	}
}

void UOmniAnimBridgeComponent::ApplyOmniState()
{
	if (!OmniAnimInstance.IsValid() || !HostPawn.IsValid())
	{
		return;
	}

	DebugSpeed = HostPawn->GetVelocity().Size2D();
	bDebugIsSprinting = MovementSystem.IsValid() && MovementSystem->IsSprinting();
	bDebugIsExhausted = StatusSystem.IsValid() && StatusSystem->IsExhausted();

	FOmniAnimBridgeFrame BridgeFrame;
	BridgeFrame.Speed = DebugSpeed;
	BridgeFrame.bIsSprinting = bDebugIsSprinting;
	BridgeFrame.bIsExhausted = bDebugIsExhausted;
	BridgeFrame.ActiveAnimSetTag = DefaultAnimSetTag;
	ApplyBodyFallbackSlots(BridgeFrame);

	OmniAnimInstance->ApplyBridgeFrame(BridgeFrame);
}

void UOmniAnimBridgeComponent::ApplyBodyFallbackSlots(FOmniAnimBridgeFrame& InOutBridgeFrame) const
{
	if (!OmniAnimInstance.IsValid() || !HostPawn.IsValid())
	{
		return;
	}

	bool bIsInAir = false;
	bool bIsCrouching = false;
	float VerticalSpeed = HostPawn->GetVelocity().Z;

	if (CharacterMovement.IsValid())
	{
		bIsInAir = CharacterMovement->IsFalling();
		bIsCrouching = CharacterMovement->IsCrouching();
		VerticalSpeed = CharacterMovement->Velocity.Z;
	}
	else if (ACharacter* Character = Cast<ACharacter>(HostPawn.Get()))
	{
		bIsCrouching = Character->bIsCrouched;
	}

	InOutBridgeFrame.bIsInAir = bIsInAir;
	InOutBridgeFrame.bIsCrouching = bIsCrouching;
	InOutBridgeFrame.VerticalSpeed = VerticalSpeed;
}
