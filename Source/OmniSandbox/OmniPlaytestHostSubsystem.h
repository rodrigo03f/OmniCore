#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OmniPlaytestHostSubsystem.generated.h"

class APawn;
class APlayerController;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class OMNISANDBOX_API UOmniPlaytestHostSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool Tick(float DeltaTime);
	void EnsureRuntimeInputAssets();
	void HandlePawnChanged(APlayerController* NewController, APawn* NewPawn);
	void ResetInputBindingState();
	void RemoveRuntimeMappingFromController(APlayerController* Controller) const;
	void EnsurePlayableCamera(APawn* Pawn) const;
	void EnsurePlayableMovementDefaults(APawn* Pawn) const;
	void EnsureInputModeGameOnly(APlayerController* Controller) const;
	void EnsureRuntimeMappingApplied(APlayerController* Controller);
	void EnsureRuntimeBindings(APlayerController* Controller);
	void LogHostReady();

	void HandleMoveForward(const FInputActionValue& Value);
	void HandleMoveRight(const FInputActionValue& Value);
	void HandleLookYaw(const FInputActionValue& Value);
	void HandleLookPitch(const FInputActionValue& Value);

private:
	FTSTicker::FDelegateHandle TickDelegateHandle;

	TWeakObjectPtr<APlayerController> ActiveController;
	TWeakObjectPtr<APawn> ActivePawn;
	TWeakObjectPtr<UEnhancedInputComponent> BoundEnhancedInputComponent;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeMappingContext = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveForwardAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveRightAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookYawAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookPitchAction = nullptr;

	bool bRuntimeMappingApplied = false;
	bool bRuntimeBindingsApplied = false;
	bool bLoggedHostReady = false;
};
