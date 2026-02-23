# OmniRuntime / OmniActionGateSystem

## Arquivos agrupados
- Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/OmniActionGateSystem.cpp
- Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateSystem.h

## Header: Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateSystem.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Systems/OmniRuntimeSystem.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "OmniActionGateSystem.generated.h"

class UOmniManifest;
class UOmniStatusSystem;
class UOmniSystemRegistrySubsystem;
class UOmniDebugSubsystem;

UCLASS()
class OMNIRUNTIME_API UOmniActionGateSystem : public UOmniRuntimeSystem
{
	GENERATED_BODY()

public:
	virtual FName GetSystemId_Implementation() const override;
	virtual TArray<FName> GetDependencies_Implementation() const override;
	virtual void InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest) override;
	virtual void ShutdownSystem_Implementation() override;
	virtual bool IsTickEnabled_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	bool TryStartAction(FName ActionId, FOmniActionGateDecision& OutDecision);

	UFUNCTION(BlueprintCallable, Category = "Omni|ActionGate")
	bool StopAction(FName ActionId, FName Reason = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	bool IsActionActive(FName ActionId) const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	TArray<FName> GetActiveActions() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	FGameplayTagContainer GetActiveLocks() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	TArray<FName> GetKnownActionIds() const;

	UFUNCTION(BlueprintPure, Category = "Omni|ActionGate")
	FOmniActionGateDecision GetLastDecision() const;

private:
	void BuildDefaultDefinitionsIfNeeded();
	void RebuildDefinitionMap();
	FGameplayTagContainer BuildCurrentBlockingContext() const;
	UOmniStatusSystem* ResolveStatusSystem() const;
	void AddActionLocks(const FOmniActionDefinition& Definition);
	void RemoveActionLocks(const FOmniActionDefinition& Definition);
	void PublishTelemetry();
	void PublishDecision(const FOmniActionGateDecision& Decision, bool bEmitLogEntry);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Omni|ActionGate")
	TArray<FOmniActionDefinition> DefaultDefinitions;

	UPROPERTY(Transient)
	TMap<FName, FOmniActionDefinition> DefinitionsById;

	UPROPERTY(Transient)
	TSet<FName> ActiveActions;

	UPROPERTY(Transient)
	TMap<FGameplayTag, int32> ActiveLockRefCounts;

	UPROPERTY(Transient)
	FOmniActionGateDecision LastDecision;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniSystemRegistrySubsystem> Registry;

	UPROPERTY(Transient)
	TWeakObjectPtr<UOmniDebugSubsystem> DebugSubsystem;

	UPROPERTY(Transient)
	bool bInitialized = false;
};

```

## Source: Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/OmniActionGateSystem.cpp
```cpp
#include "Systems/ActionGate/OmniActionGateSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "Manifest/OmniManifest.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/Status/OmniStatusSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniActionGateSystem, Log, All);

namespace OmniActionGate
{
	static const FName CategoryName(TEXT("ActionGate"));
	static const FName SourceName(TEXT("ActionGateSystem"));
}

FName UOmniActionGateSystem::GetSystemId_Implementation() const
{
	return TEXT("ActionGate");
}

TArray<FName> UOmniActionGateSystem::GetDependencies_Implementation() const
{
	return { TEXT("Status") };
}

void UOmniActionGateSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
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

	BuildDefaultDefinitionsIfNeeded();
	RebuildDefinitionMap();
	ActiveActions.Reset();
	ActiveLockRefCounts.Reset();
	LastDecision = FOmniActionGateDecision();
	bInitialized = true;

	PublishTelemetry();

	UE_LOG(
		LogOmniActionGateSystem,
		Log,
		TEXT("ActionGate system initialized. Definitions=%d Manifest=%s"),
		DefinitionsById.Num(),
		*GetNameSafe(Manifest)
	);

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->LogEvent(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("ActionGate inicializado. Definicoes=%d"), DefinitionsById.Num()),
			OmniActionGate::SourceName
		);
	}
}

void UOmniActionGateSystem::ShutdownSystem_Implementation()
{
	bInitialized = false;
	DefinitionsById.Reset();
	ActiveActions.Reset();
	ActiveLockRefCounts.Reset();
	LastDecision = FOmniActionGateDecision();

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.KnownActions"));
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.ActiveActions"));
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.ActiveLocks"));
		DebugSubsystem->RemoveMetric(TEXT("ActionGate.LastDecision"));
		DebugSubsystem->LogEvent(OmniActionGate::CategoryName, TEXT("ActionGate finalizado"), OmniActionGate::SourceName);
	}

	DebugSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniActionGateSystem, Log, TEXT("ActionGate system shutdown."));
}

bool UOmniActionGateSystem::IsTickEnabled_Implementation() const
{
	return false;
}

bool UOmniActionGateSystem::TryStartAction(const FName ActionId, FOmniActionGateDecision& OutDecision)
{
	FOmniActionGateDecision Decision;
	Decision.ActionId = ActionId;

	if (!bInitialized || ActionId == NAME_None)
	{
		Decision.bAllowed = false;
		Decision.Reason = TEXT("Sistema nao inicializado ou ActionId invalido.");
		PublishDecision(Decision, true);
		OutDecision = Decision;
		return false;
	}

	const FOmniActionDefinition* Definition = DefinitionsById.Find(ActionId);
	if (!Definition || !Definition->bEnabled)
	{
		Decision.bAllowed = false;
		Decision.Reason = TEXT("Acao desconhecida ou desabilitada.");
		PublishDecision(Decision, true);
		OutDecision = Decision;
		return false;
	}

	Decision.Policy = Definition->Policy;

	const bool bAlreadyActive = ActiveActions.Contains(ActionId);
	if (bAlreadyActive)
	{
		if (Definition->Policy == EOmniActionPolicy::DenyIfActive)
		{
			Decision.bAllowed = false;
			Decision.Reason = TEXT("Acao ja ativa (policy deny).");
			PublishDecision(Decision, true);
			OutDecision = Decision;
			return false;
		}

		if (Definition->Policy == EOmniActionPolicy::SucceedIfActive)
		{
			Decision.bAllowed = true;
			Decision.Reason = TEXT("Acao ja ativa (policy succeed).");
			PublishDecision(Decision, false);
			OutDecision = Decision;
			return true;
		}

		if (Definition->Policy == EOmniActionPolicy::RestartIfActive)
		{
			StopAction(ActionId, TEXT("RestartPolicy"));
		}
	}

	const FGameplayTagContainer BlockingContext = BuildCurrentBlockingContext();
	if (Definition->BlockedBy.HasAny(BlockingContext))
	{
		Decision.bAllowed = false;
		Decision.Reason = FString::Printf(TEXT("Bloqueada por tags: %s"), *Definition->BlockedBy.ToStringSimple());
		PublishDecision(Decision, true);
		OutDecision = Decision;
		return false;
	}

	for (const FName ActionToCancel : Definition->Cancels)
	{
		if (ActionToCancel == NAME_None || !ActiveActions.Contains(ActionToCancel))
		{
			continue;
		}

		if (StopAction(ActionToCancel, ActionId))
		{
			Decision.CanceledActions.Add(ActionToCancel);
		}
	}

	ActiveActions.Add(ActionId);
	AddActionLocks(*Definition);

	Decision.bAllowed = true;
	Decision.Reason = TEXT("Autorizada.");
	PublishDecision(Decision, true);
	OutDecision = Decision;
	return true;
}

bool UOmniActionGateSystem::StopAction(const FName ActionId, const FName Reason)
{
	if (!ActiveActions.Contains(ActionId))
	{
		return false;
	}

	const FOmniActionDefinition* Definition = DefinitionsById.Find(ActionId);
	if (!Definition)
	{
		ActiveActions.Remove(ActionId);
		PublishTelemetry();
		return true;
	}

	RemoveActionLocks(*Definition);
	ActiveActions.Remove(ActionId);
	PublishTelemetry();

	if (DebugSubsystem.IsValid())
	{
		const FString StopReason = Reason == NAME_None ? TEXT("Manual") : Reason.ToString();
		DebugSubsystem->LogEvent(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("Acao parada: %s (Reason=%s)"), *ActionId.ToString(), *StopReason),
			OmniActionGate::SourceName
		);
	}

	return true;
}

bool UOmniActionGateSystem::IsActionActive(const FName ActionId) const
{
	return ActiveActions.Contains(ActionId);
}

TArray<FName> UOmniActionGateSystem::GetActiveActions() const
{
	TArray<FName> Result = ActiveActions.Array();
	Result.Sort(FNameLexicalLess());
	return Result;
}

FGameplayTagContainer UOmniActionGateSystem::GetActiveLocks() const
{
	FGameplayTagContainer Result;
	for (const TPair<FGameplayTag, int32>& Pair : ActiveLockRefCounts)
	{
		if (Pair.Key.IsValid() && Pair.Value > 0)
		{
			Result.AddTag(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> UOmniActionGateSystem::GetKnownActionIds() const
{
	TArray<FName> Result;
	DefinitionsById.GenerateKeyArray(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

FOmniActionGateDecision UOmniActionGateSystem::GetLastDecision() const
{
	return LastDecision;
}

void UOmniActionGateSystem::BuildDefaultDefinitionsIfNeeded()
{
	if (DefaultDefinitions.Num() > 0)
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

	DefaultDefinitions.Add(MoveTemp(SprintAction));
}

void UOmniActionGateSystem::RebuildDefinitionMap()
{
	DefinitionsById.Reset();

	for (const FOmniActionDefinition& Definition : DefaultDefinitions)
	{
		if (Definition.ActionId == NAME_None)
		{
			continue;
		}

		if (DefinitionsById.Contains(Definition.ActionId))
		{
			UE_LOG(
				LogOmniActionGateSystem,
				Warning,
				TEXT("ActionId duplicado em definicoes: %s (ultima definicao vence)."),
				*Definition.ActionId.ToString()
			);
		}

		DefinitionsById.Add(Definition.ActionId, Definition);
	}
}

FGameplayTagContainer UOmniActionGateSystem::BuildCurrentBlockingContext() const
{
	FGameplayTagContainer Context;

	if (UOmniStatusSystem* StatusSystem = ResolveStatusSystem())
	{
		Context.AppendTags(StatusSystem->GetStateTags());
	}

	Context.AppendTags(GetActiveLocks());
	return Context;
}

UOmniStatusSystem* UOmniActionGateSystem::ResolveStatusSystem() const
{
	if (!Registry.IsValid())
	{
		return nullptr;
	}

	return Cast<UOmniStatusSystem>(Registry->GetSystemById(TEXT("Status")));
}

void UOmniActionGateSystem::AddActionLocks(const FOmniActionDefinition& Definition)
{
	for (const FGameplayTag& LockTag : Definition.AppliesLocks)
	{
		if (!LockTag.IsValid())
		{
			continue;
		}

		int32& Count = ActiveLockRefCounts.FindOrAdd(LockTag);
		Count++;
	}
}

void UOmniActionGateSystem::RemoveActionLocks(const FOmniActionDefinition& Definition)
{
	for (const FGameplayTag& LockTag : Definition.AppliesLocks)
	{
		if (!LockTag.IsValid())
		{
			continue;
		}

		int32* CountPtr = ActiveLockRefCounts.Find(LockTag);
		if (!CountPtr)
		{
			continue;
		}

		*CountPtr = FMath::Max(0, *CountPtr - 1);
		if (*CountPtr == 0)
		{
			ActiveLockRefCounts.Remove(LockTag);
		}
	}
}

void UOmniActionGateSystem::PublishTelemetry()
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("ActionGate.KnownActions"), FString::FromInt(DefinitionsById.Num()));
	DebugSubsystem->SetMetric(TEXT("ActionGate.ActiveActions"), FString::FromInt(ActiveActions.Num()));
	DebugSubsystem->SetMetric(TEXT("ActionGate.ActiveLocks"), FString::FromInt(ActiveLockRefCounts.Num()));
}

void UOmniActionGateSystem::PublishDecision(const FOmniActionGateDecision& Decision, const bool bEmitLogEntry)
{
	LastDecision = Decision;
	PublishTelemetry();

	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	const FString DecisionValue = FString::Printf(
		TEXT("%s | %s"),
		Decision.bAllowed ? TEXT("ALLOW") : TEXT("DENY"),
		*Decision.Reason
	);
	DebugSubsystem->SetMetric(TEXT("ActionGate.LastDecision"), DecisionValue);

	if (!bEmitLogEntry)
	{
		return;
	}

	if (Decision.bAllowed)
	{
		DebugSubsystem->LogEvent(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("ALLOW %s (%s)"), *Decision.ActionId.ToString(), *Decision.Reason),
			OmniActionGate::SourceName
		);
	}
	else
	{
		DebugSubsystem->LogWarning(
			OmniActionGate::CategoryName,
			FString::Printf(TEXT("DENY %s (%s)"), *Decision.ActionId.ToString(), *Decision.Reason),
			OmniActionGate::SourceName
		);
	}
}

```

