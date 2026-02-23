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
