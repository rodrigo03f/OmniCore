#include "Systems/ActionGate/OmniActionGateSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Manifest/OmniManifest.h"
#include "Systems/OmniSystemRegistrySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniActionGateSystem, Log, All);

namespace OmniActionGate
{
	static const FName CategoryName(TEXT("ActionGate"));
	static const FName SourceName(TEXT("ActionGateSystem"));
	static const FName SystemId(TEXT("ActionGate"));
	static const FName CommandStartAction(TEXT("StartAction"));
	static const FName CommandStopAction(TEXT("StopAction"));
	static const FName QueryCanStartAction(TEXT("CanStartAction"));
	static const FName QueryIsActionActive(TEXT("IsActionActive"));
	static const FName QueryGetStateTagsCsv(TEXT("GetStateTagsCsv"));
	static const FName StatusSystemId(TEXT("Status"));

	static FString DecisionToResult(const FOmniActionGateDecision& Decision)
	{
		return FString::Printf(TEXT("%s | %s"), Decision.bAllowed ? TEXT("ALLOW") : TEXT("DENY"), *Decision.Reason);
	}
}

FName UOmniActionGateSystem::GetSystemId_Implementation() const
{
	return TEXT("ActionGate");
}

TArray<FName> UOmniActionGateSystem::GetDependencies_Implementation() const
{
	return { OmniActionGate::StatusSystemId };
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

bool UOmniActionGateSystem::HandleCommand_Implementation(const FOmniCommandMessage& Command)
{
	if (Command.CommandName == OmniActionGate::CommandStartAction)
	{
		FName ActionId = NAME_None;
		if (!TryParseActionId(Command.Arguments, ActionId))
		{
			return false;
		}

		FOmniActionGateDecision Decision;
		return TryStartAction(ActionId, Decision);
	}

	if (Command.CommandName == OmniActionGate::CommandStopAction)
	{
		FName ActionId = NAME_None;
		if (!TryParseActionId(Command.Arguments, ActionId))
		{
			return false;
		}

		const FString* Reason = Command.Arguments.Find(TEXT("Reason"));
		return StopAction(ActionId, Reason ? FName(**Reason) : NAME_None);
	}

	return Super::HandleCommand_Implementation(Command);
}

bool UOmniActionGateSystem::HandleQuery_Implementation(FOmniQueryMessage& Query)
{
	if (Query.QueryName == OmniActionGate::QueryCanStartAction)
	{
		FName ActionId = NAME_None;
		if (!TryParseActionId(Query.Arguments, ActionId))
		{
			Query.bHandled = true;
			Query.bSuccess = false;
			Query.Result = TEXT("Missing ActionId");
			return true;
		}

		FOmniActionGateDecision Decision;
		const bool bAllowed = EvaluateStartAction(ActionId, Decision, false);
		Query.bHandled = true;
		Query.bSuccess = bAllowed;
		Query.Result = OmniActionGate::DecisionToResult(Decision);
		Query.Output.Add(TEXT("Reason"), Decision.Reason);
		Query.Output.Add(TEXT("ActionId"), ActionId.ToString());
		Query.Output.Add(TEXT("Policy"), StaticEnum<EOmniActionPolicy>()->GetNameStringByValue(static_cast<int64>(Decision.Policy)));
		return true;
	}

	if (Query.QueryName == OmniActionGate::QueryIsActionActive)
	{
		FName ActionId = NAME_None;
		if (!TryParseActionId(Query.Arguments, ActionId))
		{
			Query.bHandled = true;
			Query.bSuccess = false;
			Query.Result = TEXT("Missing ActionId");
			return true;
		}

		const bool bActive = ActiveActions.Contains(ActionId);
		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = bActive ? TEXT("True") : TEXT("False");
		return true;
	}

	return Super::HandleQuery_Implementation(Query);
}

void UOmniActionGateSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);
	(void)Event;
}

bool UOmniActionGateSystem::TryStartAction(const FName ActionId, FOmniActionGateDecision& OutDecision)
{
	const bool bAllowed = EvaluateStartAction(ActionId, OutDecision, true);
	PublishDecision(OutDecision, true);
	return bAllowed;
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

	UE_LOG(LogOmniActionGateSystem, Warning, TEXT("Action definitions loaded from DEV_FALLBACK C++."));

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

	if (Registry.IsValid())
	{
		FOmniQueryMessage Query;
		Query.SourceSystem = OmniActionGate::SystemId;
		Query.TargetSystem = OmniActionGate::StatusSystemId;
		Query.QueryName = OmniActionGate::QueryGetStateTagsCsv;

		if (Registry->ExecuteQuery(Query) && Query.bSuccess && !Query.Result.IsEmpty())
		{
			TArray<FString> TagStrings;
			Query.Result.ParseIntoArray(TagStrings, TEXT(","), true);
			for (FString TagString : TagStrings)
			{
				TagString.TrimStartAndEndInline();
				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
				if (Tag.IsValid())
				{
					Context.AddTag(Tag);
				}
			}
		}
	}

	Context.AppendTags(GetActiveLocks());
	return Context;
}

bool UOmniActionGateSystem::EvaluateStartAction(const FName ActionId, FOmniActionGateDecision& OutDecision, const bool bApplyChanges)
{
	FOmniActionGateDecision Decision;
	Decision.ActionId = ActionId;

	if (!bInitialized || ActionId == NAME_None)
	{
		Decision.bAllowed = false;
		Decision.Reason = TEXT("Sistema nao inicializado ou ActionId invalido.");
		OutDecision = Decision;
		return false;
	}

	const FOmniActionDefinition* Definition = DefinitionsById.Find(ActionId);
	if (!Definition || !Definition->bEnabled)
	{
		Decision.bAllowed = false;
		Decision.Reason = TEXT("Acao desconhecida ou desabilitada.");
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
			OutDecision = Decision;
			return false;
		}

		if (Definition->Policy == EOmniActionPolicy::SucceedIfActive)
		{
			Decision.bAllowed = true;
			Decision.Reason = TEXT("Acao ja ativa (policy succeed).");
			OutDecision = Decision;
			return true;
		}

		if (Definition->Policy == EOmniActionPolicy::RestartIfActive)
		{
			if (bApplyChanges)
			{
				StopAction(ActionId, TEXT("RestartPolicy"));
			}
			Decision.Reason = TEXT("Acao reiniciada (policy restart).");
		}
	}

	const FGameplayTagContainer BlockingContext = BuildCurrentBlockingContext();
	if (Definition->BlockedBy.HasAny(BlockingContext))
	{
		Decision.bAllowed = false;
		Decision.Reason = FString::Printf(TEXT("Bloqueada por tags: %s"), *Definition->BlockedBy.ToStringSimple());
		OutDecision = Decision;
		return false;
	}

	if (bApplyChanges)
	{
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
	}

	Decision.bAllowed = true;
	if (Decision.Reason.IsEmpty())
	{
		Decision.Reason = TEXT("Autorizada.");
	}

	OutDecision = Decision;
	return true;
}

bool UOmniActionGateSystem::TryParseActionId(const TMap<FName, FString>& Arguments, FName& OutActionId)
{
	const FString* ActionValue = Arguments.Find(TEXT("ActionId"));
	if (!ActionValue || ActionValue->IsEmpty())
	{
		return false;
	}

	OutActionId = FName(**ActionValue);
	return OutActionId != NAME_None;
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

	DebugSubsystem->SetMetric(TEXT("ActionGate.LastDecision"), OmniActionGate::DecisionToResult(Decision));

	if (Registry.IsValid())
	{
		FOmniEventMessage Event;
		Event.SourceSystem = OmniActionGate::SystemId;
		Event.EventName = Decision.bAllowed ? TEXT("ActionAllowed") : TEXT("ActionDenied");
		Event.Payload.Add(TEXT("ActionId"), Decision.ActionId.ToString());
		Event.Payload.Add(TEXT("Reason"), Decision.Reason);
		Registry->BroadcastEvent(Event);
	}

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
