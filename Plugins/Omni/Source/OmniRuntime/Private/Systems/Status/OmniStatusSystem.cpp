#include "Systems/Status/OmniStatusSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Manifest/OmniManifest.h"
#include "Systems/Attributes/OmniAttributesSystem.h"
#include "Systems/OmniClockSubsystem.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "Systems/OmniSystemRegistrySubsystem.h"

#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogOmniStatusSystem, Log, All);

namespace OmniStatus
{
	static const FName CategoryName(TEXT("Status"));
	static const FName SourceName(TEXT("StatusSystem"));
	static const FName SystemId(TEXT("Status"));
	static const FName AttributesSystemId(TEXT("Attributes"));
	static const FName CommandApplyStatus(TEXT("ApplyStatus"));
	static const FName CommandRemoveStatus(TEXT("RemoveStatus"));
	static const FName QueryGetStatusSnapshotCsv(TEXT("GetStatusSnapshotCsv"));
	static const FName DebugMetricProfileStatus(TEXT("Omni.Profile.Status"));
	static const FName StatusHasteTagName(TEXT("Game.Status.Haste"));
	static const FName StatusBurningTagName(TEXT("Game.Status.Burning"));
	static const FName ModifiersSystemId(TEXT("Modifiers"));
	static const FName ModifierCommandAdd(TEXT("AddModifier"));
	static const FName ModifierCommandRemove(TEXT("RemoveModifier"));
	static const FName ModifierHasteTagName(TEXT("Game.Modifier.HasteSpeed"));
	static const FName ModifierMovementTargetTagName(TEXT("Game.ModTarget.MovementSpeed"));
	static constexpr float HasteMovementSpeedMultiplier = 1.20f;
	static const FName DefaultStatusSource(TEXT("Status.Runtime"));
}

FName UOmniStatusSystem::GetSystemId_Implementation() const
{
	return OmniStatus::SystemId;
}

TArray<FName> UOmniStatusSystem::GetDependencies_Implementation() const
{
	return { OmniStatus::AttributesSystemId };
}

void UOmniStatusSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);
	SetInitializationResult(false);

	Registry = Cast<UOmniSystemRegistrySubsystem>(WorldContextObject);
	if (Registry.IsValid())
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>();
			ClockSubsystem = GameInstance->GetSubsystem<UOmniClockSubsystem>();
		}
	}

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Pending"));
	}

	if (!ClockSubsystem.IsValid())
	{
		const FString ClockError = TEXT("[Omni][Status][Init] OmniClock obrigatorio nao encontrado.");
		UE_LOG(LogOmniStatusSystem, Error, TEXT("%s"), *ClockError);
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Failed"));
			DebugSubsystem->LogError(OmniStatus::CategoryName, ClockError, OmniStatus::SourceName);
		}
		return;
	}

	if (!ResolveAttributesSystem())
	{
		const FString AttributesError = TEXT("[Omni][Status][Init] SYS_Attributes obrigatorio nao encontrado.");
		UE_LOG(LogOmniStatusSystem, Error, TEXT("%s"), *AttributesError);
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Failed"));
			DebugSubsystem->LogError(OmniStatus::CategoryName, AttributesError, OmniStatus::SourceName);
		}
		return;
	}

	InitializeStatusRecipes();
	ActiveStatusEffects.Reset();
	ContextTags.Reset();
	StateTags.Reset();
	LastClockSeconds = ClockSubsystem->GetSimTime();
	RefreshStatusTags();
	RefreshCombinedStateTags();
	RebuildStatusSnapshot(LastClockSeconds);
	PublishTelemetry();
	SetInitializationResult(true);

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Loaded"));
	}

	UE_LOG(LogOmniStatusSystem, Log, TEXT("[Omni][Status][Init] Inicializado | manifest=%s"), *GetNameSafe(Manifest));
}

void UOmniStatusSystem::ShutdownSystem_Implementation()
{
	StatusRecipesByTag.Reset();
	StatusSnapshot.Reset();
	ActiveStatusEffects.Reset();
	ContextTags.Reset();
	StateTags.Reset();
	LastClockSeconds = 0.0;
	AttributesSystem.Reset();

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("Status.ActiveEffects"));
		DebugSubsystem->RemoveMetric(OmniStatus::DebugMetricProfileStatus);
		DebugSubsystem->LogEvent(OmniStatus::CategoryName, TEXT("Status finalizado"), OmniStatus::SourceName);
	}

	DebugSubsystem.Reset();
	ClockSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniStatusSystem, Log, TEXT("[Omni][Status][Shutdown] Concluido"));
}

bool UOmniStatusSystem::IsTickEnabled_Implementation() const
{
	return true;
}

void UOmniStatusSystem::TickSystem_Implementation(const float DeltaTime)
{
	(void)DeltaTime;

	float ClockDeltaTime = 0.0f;
	if (!ResolveClockDelta(ClockDeltaTime))
	{
		return;
	}

	if (ClockDeltaTime > 0.0f)
	{
		TickStatusEffects(LastClockSeconds);
	}

	RefreshCombinedStateTags();
	RebuildStatusSnapshot(LastClockSeconds);
	PublishTelemetry();
}

bool UOmniStatusSystem::HandleCommand_Implementation(const FOmniCommandMessage& Command)
{
	if (Command.CommandName == OmniStatus::CommandApplyStatus)
	{
		FString StatusTagValue;
		if (!Command.TryGetArgument(TEXT("StatusTag"), StatusTagValue))
		{
			return false;
		}

		const FGameplayTag StatusTag = FGameplayTag::RequestGameplayTag(FName(*StatusTagValue), false);
		if (!StatusTag.IsValid())
		{
			return false;
		}

		FString SourceIdValue;
		FName SourceId = OmniStatus::DefaultStatusSource;
		if (Command.TryGetArgument(TEXT("SourceId"), SourceIdValue) && !SourceIdValue.IsEmpty())
		{
			SourceId = FName(*SourceIdValue);
		}

		float DurationOverrideSeconds = -1.0f;
		FString DurationValue;
		if (Command.TryGetArgument(TEXT("Duration"), DurationValue))
		{
			LexFromString(DurationOverrideSeconds, *DurationValue);
		}

		return ApplyStatus(StatusTag, SourceId, DurationOverrideSeconds);
	}

	if (Command.CommandName == OmniStatus::CommandRemoveStatus)
	{
		FString StatusTagValue;
		if (!Command.TryGetArgument(TEXT("StatusTag"), StatusTagValue))
		{
			return false;
		}

		const FGameplayTag StatusTag = FGameplayTag::RequestGameplayTag(FName(*StatusTagValue), false);
		if (!StatusTag.IsValid())
		{
			return false;
		}

		FString SourceIdValue;
		FName SourceId = OmniStatus::DefaultStatusSource;
		if (Command.TryGetArgument(TEXT("SourceId"), SourceIdValue) && !SourceIdValue.IsEmpty())
		{
			SourceId = FName(*SourceIdValue);
		}

		return RemoveStatus(StatusTag, SourceId);
	}

	return Super::HandleCommand_Implementation(Command);
}

bool UOmniStatusSystem::HandleQuery_Implementation(FOmniQueryMessage& Query)
{
	if (Query.QueryName == OmniMessageSchema::QueryGetStateTagsCsv)
	{
		RefreshCombinedStateTags();

		TArray<FString> Tags;
		for (const FGameplayTag& Tag : StateTags)
		{
			if (Tag.IsValid())
			{
				Tags.Add(Tag.ToString());
			}
		}
		Tags.Sort();

		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = FString::Join(Tags, TEXT(","));
		return true;
	}

	if (Query.QueryName == OmniStatus::QueryGetStatusSnapshotCsv)
	{
		TArray<FString> Entries;
		Entries.Reserve(StatusSnapshot.Num());
		for (const FOmniStatusEntry& Entry : StatusSnapshot)
		{
			Entries.Add(
				FString::Printf(
					TEXT("%s|%s|%d|%.2f"),
					Entry.StatusTag.IsValid() ? *Entry.StatusTag.ToString() : TEXT("<none>"),
					Entry.SourceId == NAME_None ? TEXT("<none>") : *Entry.SourceId.ToString(),
					Entry.Stacks,
					Entry.RemainingTimeSeconds
				)
			);
		}
		Entries.Sort();

		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = FString::Join(Entries, TEXT(","));
		return true;
	}

	return Super::HandleQuery_Implementation(Query);
}

void UOmniStatusSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);
	(void)Event;
}

const FGameplayTagContainer& UOmniStatusSystem::GetStateTags() const
{
	return StateTags;
}

bool UOmniStatusSystem::ApplyStatus(const FGameplayTag StatusTag, FName SourceId, const float DurationOverrideSeconds)
{
	if (!StatusTag.IsValid())
	{
		return false;
	}

	if (SourceId == NAME_None)
	{
		SourceId = OmniStatus::DefaultStatusSource;
	}

	const double NowSeconds = ClockSubsystem.IsValid() ? ClockSubsystem->GetSimTime() : LastClockSeconds;

	FOmniStatusRecipe* ExistingRecipe = StatusRecipesByTag.Find(StatusTag);
	if (!ExistingRecipe)
	{
		FOmniStatusRecipe FallbackRecipe;
		FallbackRecipe.StatusTag = StatusTag;
		FallbackRecipe.DefaultDurationSeconds = 5.0f;
		FallbackRecipe.bStackable = false;
		FallbackRecipe.MaxStacks = 1;
		FallbackRecipe.RefreshPolicy = EOmniStatusRefreshPolicy::RefreshDuration;
		FallbackRecipe.TickIntervalSeconds = 0.0f;
		StatusRecipesByTag.Add(StatusTag, FallbackRecipe);
		ExistingRecipe = StatusRecipesByTag.Find(StatusTag);

		UE_LOG(
			LogOmniStatusSystem,
			Warning,
			TEXT("[Omni][Status][Event] FallbackRecipe | status=%s duration=%.2f stackable=False"),
			*StatusTag.ToString(),
			FallbackRecipe.DefaultDurationSeconds
		);
	}

	if (!ExistingRecipe)
	{
		return false;
	}

	const float ResolvedDurationSeconds = DurationOverrideSeconds >= 0.0f
		? DurationOverrideSeconds
		: ExistingRecipe->DefaultDurationSeconds;
	const bool bInfiniteDuration = ResolvedDurationSeconds <= KINDA_SMALL_NUMBER;

	const int32 ExistingIndex = FindActiveStatusIndex(StatusTag, SourceId);
	if (ExistingIndex != INDEX_NONE)
	{
		FActiveStatusEffect& Existing = ActiveStatusEffects[ExistingIndex];

		const int32 PreviousStacks = Existing.Stacks;
		if (ExistingRecipe->bStackable)
		{
			Existing.Stacks = FMath::Clamp(Existing.Stacks + 1, 1, ExistingRecipe->MaxStacks);
		}
		else
		{
			Existing.Stacks = 1;
		}

		if (!bInfiniteDuration)
		{
			if (Existing.bInfiniteDuration)
			{
				Existing.ExpireTimeSeconds = NowSeconds + static_cast<double>(ResolvedDurationSeconds);
			}
			else
			{
				switch (ExistingRecipe->RefreshPolicy)
				{
				case EOmniStatusRefreshPolicy::RefreshDuration:
					Existing.ExpireTimeSeconds = NowSeconds + static_cast<double>(ResolvedDurationSeconds);
					break;
				case EOmniStatusRefreshPolicy::ExtendDuration:
					Existing.ExpireTimeSeconds += static_cast<double>(ResolvedDurationSeconds);
					break;
				case EOmniStatusRefreshPolicy::IgnoreDuration:
					break;
				default:
					Existing.ExpireTimeSeconds = NowSeconds + static_cast<double>(ResolvedDurationSeconds);
					break;
				}
			}
		}

		Existing.bInfiniteDuration = bInfiniteDuration;
		Existing.TickIntervalSeconds = ExistingRecipe->TickIntervalSeconds;
		Existing.NextTickTimeSeconds = Existing.TickIntervalSeconds > KINDA_SMALL_NUMBER
			? NowSeconds + static_cast<double>(Existing.TickIntervalSeconds)
			: 0.0;

		if (Existing.Stacks != PreviousStacks)
		{
			UE_LOG(
				LogOmniStatusSystem,
				Log,
				TEXT("[Omni][Status][Event] StackChanged | status=%s source=%s stacks=%d"),
				*StatusTag.ToString(),
				*SourceId.ToString(),
				Existing.Stacks
			);
		}

		UE_LOG(
			LogOmniStatusSystem,
			Log,
			TEXT("[Omni][Status][Event] Apply | status=%s source=%s duration=%.2f stacks=%d"),
			*StatusTag.ToString(),
			*SourceId.ToString(),
			ResolvedDurationSeconds,
			Existing.Stacks
		);
	}
	else
	{
		FActiveStatusEffect NewEffect;
		NewEffect.StatusTag = StatusTag;
		NewEffect.SourceId = SourceId;
		NewEffect.Stacks = 1;
		NewEffect.StartTimeSeconds = NowSeconds;
		NewEffect.bInfiniteDuration = bInfiniteDuration;
		NewEffect.ExpireTimeSeconds = bInfiniteDuration
			? TNumericLimits<double>::Max()
			: (NowSeconds + static_cast<double>(ResolvedDurationSeconds));
		NewEffect.TickIntervalSeconds = ExistingRecipe->TickIntervalSeconds;
		NewEffect.NextTickTimeSeconds = NewEffect.TickIntervalSeconds > KINDA_SMALL_NUMBER
			? NowSeconds + static_cast<double>(NewEffect.TickIntervalSeconds)
			: 0.0;
		ActiveStatusEffects.Add(NewEffect);

		UE_LOG(
			LogOmniStatusSystem,
			Log,
			TEXT("[Omni][Status][Event] Apply | status=%s source=%s duration=%.2f stacks=1"),
			*StatusTag.ToString(),
			*SourceId.ToString(),
			ResolvedDurationSeconds
		);
	}

	SyncStatusDrivenModifiers(StatusTag, SourceId, true);
	SortActiveStatusEffects();
	RefreshStatusTags();
	RefreshCombinedStateTags();
	RebuildStatusSnapshot(NowSeconds);
	PublishTelemetry();
	return true;
}

bool UOmniStatusSystem::RemoveStatus(const FGameplayTag StatusTag, const FName SourceId)
{
	const FName ResolvedSourceId = SourceId == NAME_None ? OmniStatus::DefaultStatusSource : SourceId;
	const int32 ExistingIndex = FindActiveStatusIndex(StatusTag, ResolvedSourceId);
	if (ExistingIndex == INDEX_NONE)
	{
		return false;
	}

	const FActiveStatusEffect Removed = ActiveStatusEffects[ExistingIndex];
	SyncStatusDrivenModifiers(Removed.StatusTag, Removed.SourceId, false);
	ActiveStatusEffects.RemoveAt(ExistingIndex);
	SortActiveStatusEffects();
	RefreshStatusTags();
	RefreshCombinedStateTags();
	RebuildStatusSnapshot(ClockSubsystem.IsValid() ? ClockSubsystem->GetSimTime() : LastClockSeconds);
	PublishTelemetry();

	UE_LOG(
		LogOmniStatusSystem,
		Log,
		TEXT("[Omni][Status][Event] Expire | status=%s source=%s reason=Removed"),
		*Removed.StatusTag.ToString(),
		*Removed.SourceId.ToString()
	);

	return true;
}

TArray<FOmniStatusEntry> UOmniStatusSystem::GetStatusSnapshot() const
{
	return StatusSnapshot;
}

void UOmniStatusSystem::InitializeStatusRecipes()
{
	StatusRecipesByTag.Reset();

	FOmniStatusRecipe HasteRecipe;
	HasteRecipe.StatusTag = FGameplayTag::RequestGameplayTag(OmniStatus::StatusHasteTagName, false);
	HasteRecipe.DefaultDurationSeconds = 5.0f;
	HasteRecipe.bStackable = false;
	HasteRecipe.MaxStacks = 1;
	HasteRecipe.RefreshPolicy = EOmniStatusRefreshPolicy::RefreshDuration;
	HasteRecipe.TickIntervalSeconds = 0.0f;
	if (HasteRecipe.StatusTag.IsValid())
	{
		StatusRecipesByTag.Add(HasteRecipe.StatusTag, HasteRecipe);
	}

	FOmniStatusRecipe BurningRecipe;
	BurningRecipe.StatusTag = FGameplayTag::RequestGameplayTag(OmniStatus::StatusBurningTagName, false);
	BurningRecipe.DefaultDurationSeconds = 3.0f;
	BurningRecipe.bStackable = true;
	BurningRecipe.MaxStacks = 3;
	BurningRecipe.RefreshPolicy = EOmniStatusRefreshPolicy::RefreshDuration;
	BurningRecipe.TickIntervalSeconds = 1.0f;
	if (BurningRecipe.StatusTag.IsValid())
	{
		StatusRecipesByTag.Add(BurningRecipe.StatusTag, BurningRecipe);
	}
}

void UOmniStatusSystem::RebuildStatusSnapshot(const double NowSeconds)
{
	StatusSnapshot.Reset();
	StatusSnapshot.Reserve(ActiveStatusEffects.Num());

	for (const FActiveStatusEffect& Effect : ActiveStatusEffects)
	{
		FOmniStatusEntry Entry;
		Entry.StatusTag = Effect.StatusTag;
		Entry.SourceId = Effect.SourceId;
		Entry.Stacks = Effect.Stacks;
		Entry.RemainingTimeSeconds = Effect.bInfiniteDuration
			? -1.0f
			: FMath::Max(0.0f, static_cast<float>(Effect.ExpireTimeSeconds - NowSeconds));
		StatusSnapshot.Add(Entry);
	}
}

bool UOmniStatusSystem::ResolveClockDelta(float& OutDeltaTime)
{
	OutDeltaTime = 0.0f;
	if (!ClockSubsystem.IsValid())
	{
		return false;
	}

	const double CurrentClockSeconds = ClockSubsystem->GetSimTime();
	const double RawDelta = CurrentClockSeconds - LastClockSeconds;
	LastClockSeconds = CurrentClockSeconds;
	OutDeltaTime = FMath::Max(0.0f, static_cast<float>(RawDelta));
	return true;
}

void UOmniStatusSystem::TickStatusEffects(const double NowSeconds)
{
	if (ActiveStatusEffects.Num() == 0)
	{
		return;
	}

	bool bAnyChange = false;
	for (int32 Index = ActiveStatusEffects.Num() - 1; Index >= 0; --Index)
	{
		FActiveStatusEffect& Effect = ActiveStatusEffects[Index];

		if (Effect.TickIntervalSeconds > KINDA_SMALL_NUMBER && Effect.NextTickTimeSeconds > 0.0)
		{
			while (NowSeconds + KINDA_SMALL_NUMBER >= Effect.NextTickTimeSeconds)
			{
				Effect.NextTickTimeSeconds += static_cast<double>(Effect.TickIntervalSeconds);
			}
		}

		if (!Effect.bInfiniteDuration && NowSeconds + KINDA_SMALL_NUMBER >= Effect.ExpireTimeSeconds)
		{
			SyncStatusDrivenModifiers(Effect.StatusTag, Effect.SourceId, false);
			UE_LOG(
				LogOmniStatusSystem,
				Log,
				TEXT("[Omni][Status][Event] Expire | status=%s source=%s reason=Duration"),
				*Effect.StatusTag.ToString(),
				*Effect.SourceId.ToString()
			);
			ActiveStatusEffects.RemoveAt(Index);
			bAnyChange = true;
		}
	}

	if (bAnyChange)
	{
		SortActiveStatusEffects();
		RefreshStatusTags();
	}
}

int32 UOmniStatusSystem::FindActiveStatusIndex(const FGameplayTag StatusTag, const FName SourceId) const
{
	for (int32 Index = 0; Index < ActiveStatusEffects.Num(); ++Index)
	{
		const FActiveStatusEffect& Effect = ActiveStatusEffects[Index];
		if (Effect.StatusTag == StatusTag && Effect.SourceId == SourceId)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UOmniStatusSystem::SortActiveStatusEffects()
{
	ActiveStatusEffects.Sort(
		[](const FActiveStatusEffect& Left, const FActiveStatusEffect& Right)
		{
			const FString LeftStatus = Left.StatusTag.ToString();
			const FString RightStatus = Right.StatusTag.ToString();
			if (LeftStatus != RightStatus)
			{
				return LeftStatus < RightStatus;
			}

			const FString LeftSource = Left.SourceId.ToString();
			const FString RightSource = Right.SourceId.ToString();
			return LeftSource < RightSource;
		}
	);
}

void UOmniStatusSystem::RefreshStatusTags()
{
	ContextTags.Reset();
	for (const FActiveStatusEffect& Effect : ActiveStatusEffects)
	{
		if (Effect.StatusTag.IsValid())
		{
			ContextTags.AddTag(Effect.StatusTag);
		}
	}
}

void UOmniStatusSystem::RefreshCombinedStateTags()
{
	StateTags = ContextTags;

	if (const UOmniAttributesSystem* ResolvedAttributes = ResolveAttributesSystem())
	{
		StateTags.AppendTags(ResolvedAttributes->GetStateTags());
	}
}

UOmniAttributesSystem* UOmniStatusSystem::ResolveAttributesSystem() const
{
	if (AttributesSystem.IsValid())
	{
		return AttributesSystem.Get();
	}

	if (!Registry.IsValid())
	{
		return nullptr;
	}

	AttributesSystem = Cast<UOmniAttributesSystem>(Registry->GetSystemById(OmniStatus::AttributesSystemId));
	return AttributesSystem.Get();
}

void UOmniStatusSystem::SyncStatusDrivenModifiers(const FGameplayTag StatusTag, const FName SourceId, const bool bActive) const
{
	if (!Registry.IsValid())
	{
		return;
	}

	const FGameplayTag HasteStatusTag = FGameplayTag::RequestGameplayTag(OmniStatus::StatusHasteTagName, false);
	if (!HasteStatusTag.IsValid() || StatusTag != HasteStatusTag)
	{
		return;
	}

	const FGameplayTag ModifierTag = FGameplayTag::RequestGameplayTag(OmniStatus::ModifierHasteTagName, false);
	const FGameplayTag TargetTag = FGameplayTag::RequestGameplayTag(OmniStatus::ModifierMovementTargetTagName, false);
	if (!ModifierTag.IsValid() || !TargetTag.IsValid())
	{
		return;
	}

	FOmniCommandMessage Command;
	Command.SourceSystem = OmniStatus::SystemId;
	Command.TargetSystem = OmniStatus::ModifiersSystemId;
	Command.CommandName = bActive ? OmniStatus::ModifierCommandAdd : OmniStatus::ModifierCommandRemove;
	Command.SetArgument(TEXT("ModifierTag"), ModifierTag.ToString());
	Command.SetArgument(TEXT("SourceId"), BuildStatusModifierSourceId(StatusTag, SourceId).ToString());

	if (bActive)
	{
		Command.SetArgument(TEXT("TargetTag"), TargetTag.ToString());
		Command.SetArgument(TEXT("Operation"), TEXT("Mul"));
		Command.SetArgument(TEXT("Magnitude"), FString::Printf(TEXT("%.3f"), OmniStatus::HasteMovementSpeedMultiplier));
	}

	const bool bDispatched = Registry->DispatchCommand(Command);
	if (!bDispatched)
	{
		UE_LOG(
			LogOmniStatusSystem,
			Warning,
			TEXT("[Omni][Status][ModifierBridge] DispatchFailed | status=%s source=%s command=%s"),
			*StatusTag.ToString(),
			*SourceId.ToString(),
			*Command.CommandName.ToString()
		);
		return;
	}

	UE_LOG(
		LogOmniStatusSystem,
		Log,
		TEXT("[Omni][Status][ModifierBridge] Dispatch | status=%s source=%s command=%s"),
		*StatusTag.ToString(),
		*SourceId.ToString(),
		*Command.CommandName.ToString()
	);
}

FName UOmniStatusSystem::BuildStatusModifierSourceId(const FGameplayTag StatusTag, const FName SourceId) const
{
	FString SourceText = SourceId == NAME_None ? OmniStatus::DefaultStatusSource.ToString() : SourceId.ToString();
	SourceText.ReplaceInline(TEXT("."), TEXT("_"));
	SourceText.ReplaceInline(TEXT(" "), TEXT("_"));

	FString StatusText = StatusTag.ToString();
	StatusText.ReplaceInline(TEXT("."), TEXT("_"));

	const FString FinalSource = FString::Printf(TEXT("Status_%s_%s"), *StatusText, *SourceText);
	return FName(*FinalSource);
}

void UOmniStatusSystem::PublishTelemetry()
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("Status.ActiveEffects"), FString::Printf(TEXT("%d"), ActiveStatusEffects.Num()));
}
