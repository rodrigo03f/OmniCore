#include "Systems/Modifiers/OmniModifiersSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Manifest/OmniManifest.h"
#include "Systems/OmniSystemRegistrySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniModifiersSystem, Log, All);

namespace OmniModifiers
{
	static const FName SystemId(TEXT("Modifiers"));
	static const FName AttributesSystemId(TEXT("Attributes"));
	static const FName CommandAddModifier(TEXT("AddModifier"));
	static const FName CommandRemoveModifier(TEXT("RemoveModifier"));
	static const FName QueryEvaluateModifier(TEXT("EvaluateModifier"));
	static const FName QueryGetModifiersSnapshotCsv(TEXT("GetModifiersSnapshotCsv"));
	static const FName DebugMetricActiveCount(TEXT("Modifiers.ActiveCount"));
	static const FName DefaultSourceId(TEXT("Modifier.Runtime"));
}

FName UOmniModifiersSystem::GetSystemId_Implementation() const
{
	return OmniModifiers::SystemId;
}

TArray<FName> UOmniModifiersSystem::GetDependencies_Implementation() const
{
	return { OmniModifiers::AttributesSystemId };
}

void UOmniModifiersSystem::InitializeSystem_Implementation(UObject* WorldContextObject, const UOmniManifest* Manifest)
{
	Super::InitializeSystem_Implementation(WorldContextObject, Manifest);
	SetInitializationResult(false);

	if (const UOmniSystemRegistrySubsystem* Registry = Cast<UOmniSystemRegistrySubsystem>(WorldContextObject))
	{
		if (UGameInstance* GameInstance = Registry->GetGameInstance())
		{
			DebugSubsystem = GameInstance->GetSubsystem<UOmniDebugSubsystem>();
		}
	}

	ActiveModifiers.Reset();
	PublishTelemetry();
	SetInitializationResult(true);

	UE_LOG(
		LogOmniModifiersSystem,
		Log,
		TEXT("[Omni][Modifiers][Init] Inicializado | manifest=%s"),
		*GetNameSafe(Manifest)
	);
}

void UOmniModifiersSystem::ShutdownSystem_Implementation()
{
	ActiveModifiers.Reset();

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(OmniModifiers::DebugMetricActiveCount);
	}

	DebugSubsystem.Reset();

	UE_LOG(LogOmniModifiersSystem, Log, TEXT("[Omni][Modifiers][Shutdown] Concluido"));
}

bool UOmniModifiersSystem::IsTickEnabled_Implementation() const
{
	return false;
}

bool UOmniModifiersSystem::HandleCommand_Implementation(const FOmniCommandMessage& Command)
{
	if (Command.CommandName == OmniModifiers::CommandAddModifier)
	{
		FString ModifierTagValue;
		FString TargetTagValue;
		FString OperationValue;
		FString MagnitudeValue;
		if (!Command.TryGetArgument(TEXT("ModifierTag"), ModifierTagValue)
			|| !Command.TryGetArgument(TEXT("TargetTag"), TargetTagValue)
			|| !Command.TryGetArgument(TEXT("Operation"), OperationValue)
			|| !Command.TryGetArgument(TEXT("Magnitude"), MagnitudeValue))
		{
			return false;
		}

		FOmniModifierSpec Modifier;
		Modifier.ModifierTag = FGameplayTag::RequestGameplayTag(FName(*ModifierTagValue), false);
		Modifier.TargetTag = FGameplayTag::RequestGameplayTag(FName(*TargetTagValue), false);
		Modifier.Operation = OperationValue.Equals(TEXT("Mul"), ESearchCase::IgnoreCase)
			? EOmniModifierOperation::Mul
			: EOmniModifierOperation::Add;
		LexFromString(Modifier.Magnitude, *MagnitudeValue);

		FString SourceIdValue;
		if (Command.TryGetArgument(TEXT("SourceId"), SourceIdValue) && !SourceIdValue.IsEmpty())
		{
			Modifier.SourceId = FName(*SourceIdValue);
		}
		else
		{
			Modifier.SourceId = OmniModifiers::DefaultSourceId;
		}

		FString DurationValue;
		if (Command.TryGetArgument(TEXT("Duration"), DurationValue))
		{
			LexFromString(Modifier.DurationSeconds, *DurationValue);
		}

		return AddModifier(Modifier);
	}

	if (Command.CommandName == OmniModifiers::CommandRemoveModifier)
	{
		FString ModifierTagValue;
		if (!Command.TryGetArgument(TEXT("ModifierTag"), ModifierTagValue))
		{
			return false;
		}

		const FGameplayTag ModifierTag = FGameplayTag::RequestGameplayTag(FName(*ModifierTagValue), false);
		if (!ModifierTag.IsValid())
		{
			return false;
		}

		FString SourceIdValue;
		const FName SourceId = Command.TryGetArgument(TEXT("SourceId"), SourceIdValue) && !SourceIdValue.IsEmpty()
			? FName(*SourceIdValue)
			: OmniModifiers::DefaultSourceId;

		return RemoveModifier(ModifierTag, SourceId);
	}

	return Super::HandleCommand_Implementation(Command);
}

bool UOmniModifiersSystem::HandleQuery_Implementation(FOmniQueryMessage& Query)
{
	if (Query.QueryName == OmniModifiers::QueryEvaluateModifier)
	{
		FString TargetTagValue;
		FString BaseValue;
		if (!Query.TryGetArgument(TEXT("TargetTag"), TargetTagValue)
			|| !Query.TryGetArgument(TEXT("BaseValue"), BaseValue))
		{
			return false;
		}

		const FGameplayTag TargetTag = FGameplayTag::RequestGameplayTag(FName(*TargetTagValue), false);
		if (!TargetTag.IsValid())
		{
			return false;
		}

		float ParsedBaseValue = 0.0f;
		LexFromString(ParsedBaseValue, *BaseValue);

		const float ResultValue = Evaluate(TargetTag, ParsedBaseValue);
		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = FString::Printf(TEXT("%.6f"), ResultValue);
		Query.SetOutputValue(TEXT("ResultValue"), Query.Result);
		return true;
	}

	if (Query.QueryName == OmniModifiers::QueryGetModifiersSnapshotCsv)
	{
		TArray<FString> Entries;
		Entries.Reserve(ActiveModifiers.Num());
		for (const FOmniModifierSpec& Modifier : ActiveModifiers)
		{
			Entries.Add(
				FString::Printf(
					TEXT("%s|%s|%s|%s|%.6f"),
					Modifier.TargetTag.IsValid() ? *Modifier.TargetTag.ToString() : TEXT("<none>"),
					Modifier.ModifierTag.IsValid() ? *Modifier.ModifierTag.ToString() : TEXT("<none>"),
					Modifier.SourceId == NAME_None ? TEXT("<none>") : *Modifier.SourceId.ToString(),
					Modifier.Operation == EOmniModifierOperation::Mul ? TEXT("Mul") : TEXT("Add"),
					Modifier.Magnitude
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

bool UOmniModifiersSystem::AddModifier(const FOmniModifierSpec& Modifier)
{
	FString ValidationError;
	if (!Modifier.IsValid(ValidationError))
	{
		UE_LOG(
			LogOmniModifiersSystem,
			Warning,
			TEXT("[Omni][Modifiers][Event] AddRejected | reason=%s"),
			*ValidationError
		);
		return false;
	}

	const int32 ExistingIndex = FindModifierIndex(Modifier.ModifierTag, Modifier.SourceId);
	if (ExistingIndex != INDEX_NONE)
	{
		ActiveModifiers[ExistingIndex] = Modifier;
		UE_LOG(
			LogOmniModifiersSystem,
			Log,
			TEXT("[Omni][Modifiers][Event] Add | mode=Update modifier=%s target=%s op=%s magnitude=%.4f source=%s"),
			*Modifier.ModifierTag.ToString(),
			*Modifier.TargetTag.ToString(),
			Modifier.Operation == EOmniModifierOperation::Mul ? TEXT("Mul") : TEXT("Add"),
			Modifier.Magnitude,
			*Modifier.SourceId.ToString()
		);
	}
	else
	{
		ActiveModifiers.Add(Modifier);
		UE_LOG(
			LogOmniModifiersSystem,
			Log,
			TEXT("[Omni][Modifiers][Event] Add | mode=Insert modifier=%s target=%s op=%s magnitude=%.4f source=%s"),
			*Modifier.ModifierTag.ToString(),
			*Modifier.TargetTag.ToString(),
			Modifier.Operation == EOmniModifierOperation::Mul ? TEXT("Mul") : TEXT("Add"),
			Modifier.Magnitude,
			*Modifier.SourceId.ToString()
		);
	}

	SortModifiers();
	PublishTelemetry();
	return true;
}

bool UOmniModifiersSystem::RemoveModifier(const FGameplayTag ModifierTag, FName SourceId)
{
	if (!ModifierTag.IsValid())
	{
		return false;
	}

	if (SourceId == NAME_None)
	{
		SourceId = OmniModifiers::DefaultSourceId;
	}

	const int32 ExistingIndex = FindModifierIndex(ModifierTag, SourceId);
	if (ExistingIndex == INDEX_NONE)
	{
		return false;
	}

	const FOmniModifierSpec Removed = ActiveModifiers[ExistingIndex];
	ActiveModifiers.RemoveAt(ExistingIndex);
	SortModifiers();
	PublishTelemetry();

	UE_LOG(
		LogOmniModifiersSystem,
		Log,
		TEXT("[Omni][Modifiers][Event] Remove | modifier=%s target=%s source=%s"),
		*Removed.ModifierTag.ToString(),
		*Removed.TargetTag.ToString(),
		*Removed.SourceId.ToString()
	);

	return true;
}

float UOmniModifiersSystem::Evaluate(const FGameplayTag TargetTag, const float BaseValue) const
{
	if (!TargetTag.IsValid())
	{
		return BaseValue;
	}

	float Additive = 0.0f;
	for (const FOmniModifierSpec& Modifier : ActiveModifiers)
	{
		if (Modifier.TargetTag != TargetTag || Modifier.Operation != EOmniModifierOperation::Add)
		{
			continue;
		}
		Additive += Modifier.Magnitude;
	}

	float Multiplicative = 1.0f;
	for (const FOmniModifierSpec& Modifier : ActiveModifiers)
	{
		if (Modifier.TargetTag != TargetTag || Modifier.Operation != EOmniModifierOperation::Mul)
		{
			continue;
		}
		Multiplicative *= Modifier.Magnitude;
	}

	return (BaseValue + Additive) * Multiplicative;
}

TArray<FOmniModifierSpec> UOmniModifiersSystem::GetSnapshot() const
{
	return ActiveModifiers;
}

void UOmniModifiersSystem::SortModifiers()
{
	ActiveModifiers.Sort(
		[](const FOmniModifierSpec& Left, const FOmniModifierSpec& Right)
		{
			const FString LeftTarget = Left.TargetTag.ToString();
			const FString RightTarget = Right.TargetTag.ToString();
			if (LeftTarget != RightTarget)
			{
				return LeftTarget < RightTarget;
			}

			const FString LeftModifier = Left.ModifierTag.ToString();
			const FString RightModifier = Right.ModifierTag.ToString();
			if (LeftModifier != RightModifier)
			{
				return LeftModifier < RightModifier;
			}

			const FString LeftSource = Left.SourceId.ToString();
			const FString RightSource = Right.SourceId.ToString();
			if (LeftSource != RightSource)
			{
				return LeftSource < RightSource;
			}

			const int32 LeftOperation = static_cast<int32>(Left.Operation);
			const int32 RightOperation = static_cast<int32>(Right.Operation);
			if (LeftOperation != RightOperation)
			{
				return LeftOperation < RightOperation;
			}

			return Left.Magnitude < Right.Magnitude;
		}
	);
}

void UOmniModifiersSystem::PublishTelemetry() const
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(OmniModifiers::DebugMetricActiveCount, FString::Printf(TEXT("%d"), ActiveModifiers.Num()));
}

int32 UOmniModifiersSystem::FindModifierIndex(const FGameplayTag ModifierTag, const FName SourceId) const
{
	for (int32 Index = 0; Index < ActiveModifiers.Num(); ++Index)
	{
		const FOmniModifierSpec& Entry = ActiveModifiers[Index];
		if (Entry.ModifierTag == ModifierTag && Entry.SourceId == SourceId)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}
