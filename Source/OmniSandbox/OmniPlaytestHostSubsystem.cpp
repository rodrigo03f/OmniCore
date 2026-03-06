#include "OmniPlaytestHostSubsystem.h"

#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniPlaytestHost, Log, All);

namespace OmniPlaytestHost
{
	static constexpr int32 MappingPriority = 100;
	static constexpr float DefaultArmLength = 300.0f;
	static constexpr float DefaultWalkSpeed = 600.0f;
	static const FVector DefaultCameraOffset(0.0f, 0.0f, 60.0f);
	static const FRotator DefaultRotationRate(0.0f, 720.0f, 0.0f);
}

void UOmniPlaytestHostSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EnsureRuntimeInputAssets();
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UOmniPlaytestHostSubsystem::Tick)
	);
}

void UOmniPlaytestHostSubsystem::Deinitialize()
{
	RemoveRuntimeMappingFromController(ActiveController.Get());
	ResetInputBindingState();

	if (TickDelegateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}

	ActiveController.Reset();
	ActivePawn.Reset();
	RuntimeMappingContext = nullptr;
	MoveForwardAction = nullptr;
	MoveRightAction = nullptr;
	LookYawAction = nullptr;
	LookPitchAction = nullptr;

	Super::Deinitialize();
}

bool UOmniPlaytestHostSubsystem::Tick(const float DeltaTime)
{
	(void)DeltaTime;

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return true;
	}

	APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return true;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return true;
	}

	if (PlayerController != ActiveController.Get() || Pawn != ActivePawn.Get())
	{
		HandlePawnChanged(PlayerController, Pawn);
	}

	EnsurePlayableCamera(Pawn);
	EnsurePlayableMovementDefaults(Pawn);
	EnsureInputModeGameOnly(PlayerController);
	EnsureRuntimeMappingApplied(PlayerController);
	EnsureRuntimeBindings(PlayerController);
	LogHostReady();

	return true;
}

void UOmniPlaytestHostSubsystem::EnsureRuntimeInputAssets()
{
	if (!RuntimeMappingContext)
	{
		RuntimeMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_OmniPlaytestRuntime"));
	}

	if (!MoveForwardAction)
	{
		MoveForwardAction = NewObject<UInputAction>(this, TEXT("IA_OmniMoveForward"));
		MoveForwardAction->ValueType = EInputActionValueType::Axis1D;
	}

	if (!MoveRightAction)
	{
		MoveRightAction = NewObject<UInputAction>(this, TEXT("IA_OmniMoveRight"));
		MoveRightAction->ValueType = EInputActionValueType::Axis1D;
	}

	if (!LookYawAction)
	{
		LookYawAction = NewObject<UInputAction>(this, TEXT("IA_OmniLookYaw"));
		LookYawAction->ValueType = EInputActionValueType::Axis1D;
	}

	if (!LookPitchAction)
	{
		LookPitchAction = NewObject<UInputAction>(this, TEXT("IA_OmniLookPitch"));
		LookPitchAction->ValueType = EInputActionValueType::Axis1D;
	}

	if (!RuntimeMappingContext)
	{
		return;
	}

	if (RuntimeMappingContext->GetMappings().Num() > 0)
	{
		return;
	}

	(void)RuntimeMappingContext->MapKey(MoveForwardAction, EKeys::W);
	FEnhancedActionKeyMapping& BackwardMapping = RuntimeMappingContext->MapKey(MoveForwardAction, EKeys::S);
	BackwardMapping.Modifiers.Add(NewObject<UInputModifierNegate>(RuntimeMappingContext));

	(void)RuntimeMappingContext->MapKey(MoveRightAction, EKeys::D);
	FEnhancedActionKeyMapping& LeftMapping = RuntimeMappingContext->MapKey(MoveRightAction, EKeys::A);
	LeftMapping.Modifiers.Add(NewObject<UInputModifierNegate>(RuntimeMappingContext));

	(void)RuntimeMappingContext->MapKey(LookYawAction, EKeys::MouseX);
	FEnhancedActionKeyMapping& PitchMapping = RuntimeMappingContext->MapKey(LookPitchAction, EKeys::MouseY);
	PitchMapping.Modifiers.Add(NewObject<UInputModifierNegate>(RuntimeMappingContext));
}

void UOmniPlaytestHostSubsystem::HandlePawnChanged(APlayerController* NewController, APawn* NewPawn)
{
	RemoveRuntimeMappingFromController(ActiveController.Get());
	ResetInputBindingState();

	ActiveController = NewController;
	ActivePawn = NewPawn;
	bLoggedHostReady = false;

	EnsurePlayableCamera(NewPawn);
	EnsurePlayableMovementDefaults(NewPawn);
	EnsureInputModeGameOnly(NewController);
	EnsureRuntimeMappingApplied(NewController);

	UE_LOG(
		LogOmniPlaytestHost,
		Log,
		TEXT("[Omni][PlaytestHost][Pawn] Controller=%s Pawn=%s"),
		*GetNameSafe(NewController),
		*GetNameSafe(NewPawn)
	);
}

void UOmniPlaytestHostSubsystem::ResetInputBindingState()
{
	BoundEnhancedInputComponent.Reset();
	bRuntimeMappingApplied = false;
	bRuntimeBindingsApplied = false;
}

void UOmniPlaytestHostSubsystem::RemoveRuntimeMappingFromController(APlayerController* Controller) const
{
	if (!Controller || !RuntimeMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	InputSubsystem->RemoveMappingContext(RuntimeMappingContext);
}

void UOmniPlaytestHostSubsystem::EnsurePlayableCamera(APawn* Pawn) const
{
	if (!Pawn)
	{
		return;
	}

	USceneComponent* RootComponent = Pawn->GetRootComponent();
	if (!RootComponent)
	{
		return;
	}

	USpringArmComponent* SpringArm = Pawn->FindComponentByClass<USpringArmComponent>();
	if (!SpringArm)
	{
		SpringArm = NewObject<USpringArmComponent>(Pawn, TEXT("OmniPlaytestSpringArm"));
		if (SpringArm)
		{
			Pawn->AddInstanceComponent(SpringArm);
			SpringArm->SetupAttachment(RootComponent);
			SpringArm->RegisterComponent();
		}
	}

	if (SpringArm)
	{
		SpringArm->TargetArmLength = OmniPlaytestHost::DefaultArmLength;
		SpringArm->SocketOffset = OmniPlaytestHost::DefaultCameraOffset;
		SpringArm->bUsePawnControlRotation = true;
		SpringArm->bEnableCameraLag = false;
		SpringArm->CameraLagSpeed = 0.0f;
	}

	UCameraComponent* CameraComponent = Pawn->FindComponentByClass<UCameraComponent>();
	if (!CameraComponent)
	{
		CameraComponent = NewObject<UCameraComponent>(Pawn, TEXT("OmniPlaytestCamera"));
		if (CameraComponent)
		{
			Pawn->AddInstanceComponent(CameraComponent);
			if (SpringArm)
			{
				CameraComponent->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
			}
			else
			{
				CameraComponent->SetupAttachment(RootComponent);
			}
			CameraComponent->RegisterComponent();
		}
	}

	if (CameraComponent)
	{
		if (SpringArm && CameraComponent->GetAttachParent() != SpringArm)
		{
			CameraComponent->AttachToComponent(
				SpringArm,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				USpringArmComponent::SocketName
			);
		}
		CameraComponent->bUsePawnControlRotation = false;
	}
}

void UOmniPlaytestHostSubsystem::EnsurePlayableMovementDefaults(APawn* Pawn) const
{
	ACharacter* Character = Cast<ACharacter>(Pawn);
	if (!Character)
	{
		return;
	}

	Character->bUseControllerRotationYaw = false;
	Character->bUseControllerRotationPitch = false;
	Character->bUseControllerRotationRoll = false;

	UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();
	if (!CharacterMovement)
	{
		return;
	}

	if (CharacterMovement->MaxWalkSpeed <= KINDA_SMALL_NUMBER)
	{
		CharacterMovement->MaxWalkSpeed = OmniPlaytestHost::DefaultWalkSpeed;
	}
	CharacterMovement->bOrientRotationToMovement = true;
	CharacterMovement->RotationRate = OmniPlaytestHost::DefaultRotationRate;
}

void UOmniPlaytestHostSubsystem::EnsureInputModeGameOnly(APlayerController* Controller) const
{
	if (!Controller)
	{
		return;
	}

	FInputModeGameOnly InputMode;
	Controller->SetInputMode(InputMode);
	Controller->SetShowMouseCursor(false);
}

void UOmniPlaytestHostSubsystem::EnsureRuntimeMappingApplied(APlayerController* Controller)
{
	if (bRuntimeMappingApplied || !Controller || !RuntimeMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	InputSubsystem->RemoveMappingContext(RuntimeMappingContext);
	InputSubsystem->AddMappingContext(RuntimeMappingContext, OmniPlaytestHost::MappingPriority);
	bRuntimeMappingApplied = true;

	UE_LOG(
		LogOmniPlaytestHost,
		Log,
		TEXT("[Omni][PlaytestHost][Input] MappingContext aplicado | pawn=%s"),
		*GetNameSafe(ActivePawn.Get())
	);
}

void UOmniPlaytestHostSubsystem::EnsureRuntimeBindings(APlayerController* Controller)
{
	if (!Controller || !MoveForwardAction || !MoveRightAction || !LookYawAction || !LookPitchAction)
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(Controller->InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (bRuntimeBindingsApplied && BoundEnhancedInputComponent.Get() == EnhancedInputComponent)
	{
		return;
	}

	BoundEnhancedInputComponent = EnhancedInputComponent;
	bRuntimeBindingsApplied = true;

	EnhancedInputComponent->BindAction(
		MoveForwardAction,
		ETriggerEvent::Triggered,
		this,
		&UOmniPlaytestHostSubsystem::HandleMoveForward
	);
	EnhancedInputComponent->BindAction(
		MoveRightAction,
		ETriggerEvent::Triggered,
		this,
		&UOmniPlaytestHostSubsystem::HandleMoveRight
	);
	EnhancedInputComponent->BindAction(
		LookYawAction,
		ETriggerEvent::Triggered,
		this,
		&UOmniPlaytestHostSubsystem::HandleLookYaw
	);
	EnhancedInputComponent->BindAction(
		LookPitchAction,
		ETriggerEvent::Triggered,
		this,
		&UOmniPlaytestHostSubsystem::HandleLookPitch
	);

	UE_LOG(
		LogOmniPlaytestHost,
		Log,
		TEXT("[Omni][PlaytestHost][Input] Bindings WASD+Mouse ativos | pawn=%s"),
		*GetNameSafe(ActivePawn.Get())
	);
}

void UOmniPlaytestHostSubsystem::LogHostReady()
{
	if (bLoggedHostReady || !ActivePawn.IsValid())
	{
		return;
	}

	APawn* Pawn = ActivePawn.Get();
	TArray<UActorComponent*> Components;
	Pawn->GetComponents(Components);

	bool bHasAvatarBridge = false;
	bool bHasAnimBridge = false;
	for (const UActorComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		const FString ClassName = Component->GetClass()->GetName();
		if (ClassName.Contains(TEXT("OmniAvatarBridgeComponent")))
		{
			bHasAvatarBridge = true;
		}
		if (ClassName.Contains(TEXT("OmniAnimBridgeComponent")))
		{
			bHasAnimBridge = true;
		}
	}

	UE_LOG(
		LogOmniPlaytestHost,
		Log,
		TEXT("[Omni][PlaytestHost][Ready] pawn=%s springArm=%s camera=%s bindings=%s avatarBridge=%s animBridge=%s"),
		*GetNameSafe(Pawn),
		Pawn->FindComponentByClass<USpringArmComponent>() ? TEXT("True") : TEXT("False"),
		Pawn->FindComponentByClass<UCameraComponent>() ? TEXT("True") : TEXT("False"),
		bRuntimeBindingsApplied ? TEXT("True") : TEXT("False"),
		bHasAvatarBridge ? TEXT("True") : TEXT("False"),
		bHasAnimBridge ? TEXT("True") : TEXT("False")
	);

	bLoggedHostReady = true;
}

void UOmniPlaytestHostSubsystem::HandleMoveForward(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	APlayerController* Controller = ActiveController.Get();
	APawn* Pawn = ActivePawn.Get();
	if (!Controller || !Pawn)
	{
		return;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	ControlRotation.Pitch = 0.0f;
	ControlRotation.Roll = 0.0f;
	const FVector ForwardDirection = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
	Pawn->AddMovementInput(ForwardDirection, AxisValue);
}

void UOmniPlaytestHostSubsystem::HandleMoveRight(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	APlayerController* Controller = ActiveController.Get();
	APawn* Pawn = ActivePawn.Get();
	if (!Controller || !Pawn)
	{
		return;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	ControlRotation.Pitch = 0.0f;
	ControlRotation.Roll = 0.0f;
	const FVector RightDirection = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
	Pawn->AddMovementInput(RightDirection, AxisValue);
}

void UOmniPlaytestHostSubsystem::HandleLookYaw(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	if (APawn* Pawn = ActivePawn.Get())
	{
		Pawn->AddControllerYawInput(AxisValue);
	}
}

void UOmniPlaytestHostSubsystem::HandleLookPitch(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	if (APawn* Pawn = ActivePawn.Get())
	{
		Pawn->AddControllerPitchInput(AxisValue);
	}
}
