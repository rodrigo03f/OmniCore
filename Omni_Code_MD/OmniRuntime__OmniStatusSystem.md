# OmniRuntime / OmniStatusSystem

## Arquivos agrupados
- Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/OmniStatusSystem.cpp
- Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusSystem.h

## Header: Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusSystem.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/OmniRuntimeSystem.h"
#include "OmniStatusSystem.generated.h"

class UOmniManifest;
class UOmniDebugSubsystem;
class UOmniSystemRegistrySubsystem;

UCLASS()
class OMNIRUNTIME_API UOmniStatusSystem : public UOmniRuntimeSystem
{
	GENERATED_BODY()

public:
	virtual FName GetSystemId_Implementation() const override;
	virtual TArray<FName> GetDependencies_Implementation() const override;
	virtual void InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest) override;
	virtual void ShutdownSystem_Implementation() override;
	virtual bool IsTickEnabled_Implementation() const override;
	virtual void TickSystem_Implementation(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	float GetStaminaNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Omni|Status")
	bool IsExhausted() const;

	const FGameplayTagContainer& GetStateTags() const;

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	void SetSprinting(bool bInSprinting);

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	void ConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Omni|Status")
	void AddStamina(float Amount);

private:
	void UpdateStateTags();
	void PublishTelemetry();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "1.0"))
	float MaxStaminaValue = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.1"))
	float SprintDrainPerSecond = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.1"))
	float RegenPerSecond = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.0"))
	float RegenDelaySeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Omni|Status|Stamina", meta = (ClampMin = "0.0"))
	float ExhaustRecoverThreshold = 22.0f;

	UPROPERTY(Transient)
	float CurrentStamina = 100.0f;

	UPROPERTY(Transient)
	bool bSprinting = false;

	UPROPERTY(Transient)
	bool bExhausted = false;

	UPROPERTY(Transient)
	float TimeSinceLastConsumption = 0.0f;

	UPROPERTY(Transient)
	FGameplayTagContainer StateTags;

	UPROPERTY(Transient)
	FGameplayTag ExhaustedTag;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;
};

```

## Source: Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/OmniStatusSystem.cpp
```cpp
#include "Systems/Status/OmniStatusSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Manifest/OmniManifest.h"
#include "Systems/OmniSystemRegistrySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniStatusSystem, Log, All);

namespace OmniStatus
{
	static const FName CategoryName(TEXT("Status"));
	static const FName SourceName(TEXT("StatusSystem"));
}

FName UOmniStatusSystem::GetSystemId_Implementation() const
{
	return TEXT("Status");
}

TArray<FName> UOmniStatusSystem::GetDependencies_Implementation() const
{
	return {};
}

void UOmniStatusSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);

	Registry = Cast<UOmniSystemRegistrySubsystem>(WorldContextObject);
	if (Registry.IsValid())
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>();
		}
	}

	ExhaustedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Exhausted"), false);
	CurrentStamina = MaxStaminaValue;
	bExhausted = false;
	bSprinting = false;
	TimeSinceLastConsumption = RegenDelaySeconds;
	UpdateStateTags();
	PublishTelemetry();

	UE_LOG(LogOmniStatusSystem, Log, TEXT("Status system initialized. Manifest=%s"), *GetNameSafe(Manifest));

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->LogEvent(
			OmniStatus::CategoryName,
			FString::Printf(TEXT("Status inicializado. Stamina=%.1f"), CurrentStamina),
			OmniStatus::SourceName
		);
	}
}

void UOmniStatusSystem::ShutdownSystem_Implementation()
{
	bSprinting = false;
	bExhausted = false;
	StateTags.Reset();

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("Status.Stamina"));
		DebugSubsystem->RemoveMetric(TEXT("Status.Exhausted"));
		DebugSubsystem->RemoveMetric(TEXT("Status.Sprinting"));
		DebugSubsystem->LogEvent(OmniStatus::CategoryName, TEXT("Status finalizado"), OmniStatus::SourceName);
	}

	DebugSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniStatusSystem, Log, TEXT("Status system shutdown."));
}

bool UOmniStatusSystem::IsTickEnabled_Implementation() const
{
	return true;
}

void UOmniStatusSystem::TickSystem_Implementation(const float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	if (bSprinting)
	{
		ConsumeStamina(SprintDrainPerSecond * DeltaTime);
	}
	else
	{
		TimeSinceLastConsumption += DeltaTime;
		if (TimeSinceLastConsumption >= RegenDelaySeconds && CurrentStamina < MaxStaminaValue)
		{
			AddStamina(RegenPerSecond * DeltaTime);
		}
	}

	if (!bExhausted && CurrentStamina <= KINDA_SMALL_NUMBER)
	{
		bExhausted = true;
		UpdateStateTags();

		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->LogWarning(OmniStatus::CategoryName, TEXT("Entrou em estado Exhausted"), OmniStatus::SourceName);
		}
	}
	else if (bExhausted && CurrentStamina >= ExhaustRecoverThreshold)
	{
		bExhausted = false;
		UpdateStateTags();

		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->LogEvent(OmniStatus::CategoryName, TEXT("Saiu de estado Exhausted"), OmniStatus::SourceName);
		}
	}

	PublishTelemetry();
}

float UOmniStatusSystem::GetCurrentStamina() const
{
	return CurrentStamina;
}

float UOmniStatusSystem::GetMaxStamina() const
{
	return MaxStaminaValue;
}

float UOmniStatusSystem::GetStaminaNormalized() const
{
	if (MaxStaminaValue <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return CurrentStamina / MaxStaminaValue;
}

bool UOmniStatusSystem::IsExhausted() const
{
	return bExhausted;
}

const FGameplayTagContainer& UOmniStatusSystem::GetStateTags() const
{
	return StateTags;
}

void UOmniStatusSystem::SetSprinting(const bool bInSprinting)
{
	if (bSprinting == bInSprinting)
	{
		return;
	}

	bSprinting = bInSprinting;
	if (!bSprinting)
	{
		TimeSinceLastConsumption = 0.0f;
	}

	PublishTelemetry();
}

void UOmniStatusSystem::ConsumeStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentStamina = FMath::Max(0.0f, CurrentStamina - Amount);
	TimeSinceLastConsumption = 0.0f;
}

void UOmniStatusSystem::AddStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentStamina = FMath::Min(MaxStaminaValue, CurrentStamina + Amount);
}

void UOmniStatusSystem::UpdateStateTags()
{
	StateTags.Reset();
	if (bExhausted && ExhaustedTag.IsValid())
	{
		StateTags.AddTag(ExhaustedTag);
	}
}

void UOmniStatusSystem::PublishTelemetry()
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("Status.Stamina"), FString::Printf(TEXT("%.1f/%.1f"), CurrentStamina, MaxStaminaValue));
	DebugSubsystem->SetMetric(TEXT("Status.Exhausted"), bExhausted ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(TEXT("Status.Sprinting"), bSprinting ? TEXT("True") : TEXT("False"));
}

```

