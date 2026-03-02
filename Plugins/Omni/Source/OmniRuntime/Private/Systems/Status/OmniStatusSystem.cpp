#include "Systems/Status/OmniStatusSystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Manifest/OmniManifest.h"
#include "Misc/PackageName.h"
#include "Profile/OmniAttributesRecipeDataAsset.h"
#include "Systems/OmniClockSubsystem.h"
#include "Systems/OmniSystemRegistrySubsystem.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniStatusSystem, Log, All);

namespace OmniStatus
{
	static const FName CategoryName(TEXT("Attributes"));
	static const FName SourceName(TEXT("AttributesSystem"));
	static const FName SystemId(TEXT("Status"));
	static const FName CommandSetSprinting(TEXT("SetSprinting"));
	static const FName CommandConsumeStamina(TEXT("ConsumeStamina"));
	static const FName CommandAddStamina(TEXT("AddStamina"));
	static const FName QueryIsExhausted(TEXT("IsExhausted"));
	static const FName QueryGetStateTagsCsv(TEXT("GetStateTagsCsv"));
	static const FName QueryGetStamina(TEXT("GetStamina"));
	static const FName ConfigSectionAttributes(TEXT("Omni.Attributes"));
	static const FName ConfigKeyRecipe(TEXT("Recipe"));
	static const FName ConfigSectionLegacy(TEXT("/Script/OmniRuntime.OmniStatusSystem"));
	static const FName ConfigKeyRecipeFlat(TEXT("Omni.Attributes.Recipe"));
	static const TCHAR* DefaultRecipeAssetPath = TEXT("/Game/Omni/Recipes/DA_AttributesRecipe_Default.DA_AttributesRecipe_Default");
	static const FName DebugMetricProfileStatus(TEXT("Omni.Profile.Status"));
	static const FName DebugMetricProfileAttributes(TEXT("Omni.Profile.Attributes"));
	static const FName HpTagName(TEXT("Game.Attr.HP"));
	static const FName StaminaTagName(TEXT("Game.Attr.Stamina"));
	static const FName SprintingStateTagName(TEXT("Game.State.Sprinting"));
	static const FName ExhaustedStateTagName(TEXT("Game.State.Exhausted"));
}

static FString NormalizeRecipePath(const FString& RawPath)
{
	FString Path = RawPath;
	Path.TrimStartAndEndInline();
	if (Path.IsEmpty())
	{
		return FString(OmniStatus::DefaultRecipeAssetPath);
	}
	if (Path.Contains(TEXT(".")))
	{
		return Path;
	}
	const FString AssetName = FPackageName::GetShortName(Path);
	return FString::Printf(TEXT("%s.%s"), *Path, *AssetName);
}

FName UOmniStatusSystem::GetSystemId_Implementation() const
{
	return OmniStatus::SystemId;
}

TArray<FName> UOmniStatusSystem::GetDependencies_Implementation() const
{
	return {};
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
		DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileAttributes, TEXT("Pending"));
	}
	if (!ClockSubsystem.IsValid())
	{
		const FString ClockError = TEXT("[Omni][Attributes][Init] OmniClock obrigatorio nao encontrado.");
		UE_LOG(LogOmniStatusSystem, Error, TEXT("%s"), *ClockError);
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Failed"));
			DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileAttributes, TEXT("Failed"));
			DebugSubsystem->LogError(OmniStatus::CategoryName, ClockError, OmniStatus::SourceName);
		}
		return;
	}

	ResolveOfficialTags();
	ApplyFallbackRecipe();

	FString RecipeLoadError;
	const bool bLoadedRecipeFromConfig = TryLoadRecipeFromConfig(RecipeLoadError);
	if (!bLoadedRecipeFromConfig)
	{
		UE_LOG(
			LogOmniStatusSystem,
			Warning,
			TEXT("[Omni][Attributes][Init] Recipe via config indisponivel. Usando fallback deterministico | %s"),
			*RecipeLoadError
		);
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->LogWarning(OmniStatus::CategoryName, RecipeLoadError, OmniStatus::SourceName);
		}
	}

	LastClockSeconds = ClockSubsystem->GetSimTime();
	TimeSinceLastStaminaDrain = StaminaRules.RegenDelaySeconds;
	bExhausted = false;
	UpdateStateTags();
	InitializeSnapshot();
	PublishTelemetry();
	SetInitializationResult(true);

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->SetMetric(OmniStatus::DebugMetricProfileStatus, TEXT("Loaded"));
		DebugSubsystem->SetMetric(
			OmniStatus::DebugMetricProfileAttributes,
			bLoadedRecipeFromConfig ? TEXT("RecipeLoaded") : TEXT("FallbackLoaded")
		);
	}

	UE_LOG(
		LogOmniStatusSystem,
		Log,
		TEXT("[Omni][Attributes][Init] Inicializado | manifest=%s recipeMode=%s"),
		*GetNameSafe(Manifest),
		bLoadedRecipeFromConfig ? TEXT("Config") : TEXT("Fallback")
	);
}

void UOmniStatusSystem::ShutdownSystem_Implementation()
{
	AttributesByTag.Reset();
	ContextTags.Reset();
	StateTags.Reset();
	StaminaRules = FOmniStaminaRules();
	Snapshot = FOmniAttributeSnapshot();
	bExhausted = false;
	TimeSinceLastStaminaDrain = 0.0f;
	LastClockSeconds = 0.0;

	if (DebugSubsystem.IsValid())
	{
		DebugSubsystem->RemoveMetric(TEXT("Status.Stamina"));
		DebugSubsystem->RemoveMetric(TEXT("Status.Exhausted"));
		DebugSubsystem->RemoveMetric(TEXT("Status.Sprinting"));
		DebugSubsystem->RemoveMetric(TEXT("Status.HP"));
		DebugSubsystem->RemoveMetric(OmniStatus::DebugMetricProfileStatus);
		DebugSubsystem->RemoveMetric(OmniStatus::DebugMetricProfileAttributes);
		DebugSubsystem->LogEvent(OmniStatus::CategoryName, TEXT("Attributes finalizado"), OmniStatus::SourceName);
	}

	DebugSubsystem.Reset();
	ClockSubsystem.Reset();
	Registry.Reset();

	UE_LOG(LogOmniStatusSystem, Log, TEXT("[Omni][Attributes][Shutdown] Concluido"));
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
		TickStamina(ClockDeltaTime);
	}

	InitializeSnapshot();
	PublishTelemetry();
}

bool UOmniStatusSystem::HandleCommand_Implementation(const FOmniCommandMessage& Command)
{
	if (Command.CommandName == OmniMessageSchema::CommandSetSprinting)
	{
		FOmniSetSprintingCommandSchema ParsedSchema;
		FString ParseError;
		if (!FOmniSetSprintingCommandSchema::TryFromMessage(Command, ParsedSchema, ParseError))
		{
			return false;
		}

		SetSprinting(ParsedSchema.bSprinting);
		return true;
	}

	if (Command.CommandName == OmniStatus::CommandConsumeStamina)
	{
		FString AmountValue;
		if (!Command.TryGetArgument(TEXT("Amount"), AmountValue))
		{
			return false;
		}

		float Amount = 0.0f;
		LexFromString(Amount, *AmountValue);
		ConsumeStamina(Amount);
		return true;
	}

	if (Command.CommandName == OmniStatus::CommandAddStamina)
	{
		FString AmountValue;
		if (!Command.TryGetArgument(TEXT("Amount"), AmountValue))
		{
			return false;
		}

		float Amount = 0.0f;
		LexFromString(Amount, *AmountValue);
		AddStamina(Amount);
		return true;
	}

	return Super::HandleCommand_Implementation(Command);
}

bool UOmniStatusSystem::HandleQuery_Implementation(FOmniQueryMessage& Query)
{
	if (Query.QueryName == OmniStatus::QueryIsExhausted)
	{
		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = bExhausted ? TEXT("True") : TEXT("False");
		return true;
	}

	if (Query.QueryName == OmniStatus::QueryGetStateTagsCsv)
	{
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

	if (Query.QueryName == OmniStatus::QueryGetStamina)
	{
		const FOmniAttributeValue* StaminaAttribute = FindAttribute(StaminaTag);
		const float CurrentStamina = StaminaAttribute ? StaminaAttribute->Current : 0.0f;
		const float MaxStamina = StaminaAttribute ? StaminaAttribute->Max : 0.0f;

		Query.bHandled = true;
		Query.bSuccess = true;
		Query.Result = FString::Printf(TEXT("%.2f/%.2f"), CurrentStamina, MaxStamina);
		Query.SetOutputValue(TEXT("Current"), FString::Printf(TEXT("%.2f"), CurrentStamina));
		Query.SetOutputValue(TEXT("Max"), FString::Printf(TEXT("%.2f"), MaxStamina));
		Query.SetOutputValue(TEXT("Normalized"), FString::Printf(TEXT("%.4f"), GetStaminaNormalized()));
		return true;
	}

	return Super::HandleQuery_Implementation(Query);
}

void UOmniStatusSystem::HandleEvent_Implementation(const FOmniEventMessage& Event)
{
	Super::HandleEvent_Implementation(Event);
	(void)Event;
}

FOmniAttributeSnapshot UOmniStatusSystem::GetSnapshot() const
{
	return Snapshot;
}

float UOmniStatusSystem::GetCurrentStamina() const
{
	if (const FOmniAttributeValue* StaminaAttribute = FindAttribute(StaminaTag))
	{
		return StaminaAttribute->Current;
	}
	return 0.0f;
}

float UOmniStatusSystem::GetMaxStamina() const
{
	if (const FOmniAttributeValue* StaminaAttribute = FindAttribute(StaminaTag))
	{
		return StaminaAttribute->Max;
	}
	return 0.0f;
}

float UOmniStatusSystem::GetStaminaNormalized() const
{
	const float MaxStamina = GetMaxStamina();
	if (MaxStamina <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	return GetCurrentStamina() / MaxStamina;
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
	if (SprintingStateTag.IsValid())
	{
		if (bInSprinting)
		{
			ContextTags.AddTag(SprintingStateTag);
		}
		else
		{
			ContextTags.RemoveTag(SprintingStateTag);
		}
	}

	UpdateStateTags();
	InitializeSnapshot();
	PublishTelemetry();
}

void UOmniStatusSystem::ConsumeStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	FOmniAttributeValue* StaminaAttribute = FindAttributeMutable(StaminaTag);
	if (!StaminaAttribute)
	{
		return;
	}

	StaminaAttribute->Current -= Amount;
	ClampAttribute(*StaminaAttribute);
	TimeSinceLastStaminaDrain = 0.0f;
}

void UOmniStatusSystem::AddStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	FOmniAttributeValue* StaminaAttribute = FindAttributeMutable(StaminaTag);
	if (!StaminaAttribute)
	{
		return;
	}

	StaminaAttribute->Current += Amount;
	ClampAttribute(*StaminaAttribute);
}

void UOmniStatusSystem::ApplyDamage(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	FOmniAttributeValue* HpAttribute = FindAttributeMutable(HpTag);
	if (!HpAttribute)
	{
		return;
	}

	HpAttribute->Current -= Amount;
	ClampAttribute(*HpAttribute);
	InitializeSnapshot();
	PublishTelemetry();
}

void UOmniStatusSystem::ApplyHeal(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	FOmniAttributeValue* HpAttribute = FindAttributeMutable(HpTag);
	if (!HpAttribute)
	{
		return;
	}

	HpAttribute->Current += Amount;
	ClampAttribute(*HpAttribute);
	InitializeSnapshot();
	PublishTelemetry();
}

bool UOmniStatusSystem::TryLoadRecipeFromConfig(FString& OutError)
{
	OutError.Reset();

	FString RecipePath;
	bool bFoundRecipePath = GConfig->GetString(
		*OmniStatus::ConfigSectionAttributes.ToString(),
		*OmniStatus::ConfigKeyRecipe.ToString(),
		RecipePath,
		GGameIni
	);
	if (!bFoundRecipePath)
	{
		bFoundRecipePath = GConfig->GetString(
			*OmniStatus::ConfigSectionLegacy.ToString(),
			*OmniStatus::ConfigKeyRecipeFlat.ToString(),
			RecipePath,
			GGameIni
		);
	}
	if (!bFoundRecipePath || RecipePath.IsEmpty())
	{
		RecipePath = OmniStatus::DefaultRecipeAssetPath;
	}

	const FString NormalizedPath = NormalizeRecipePath(RecipePath);
	const FSoftObjectPath RecipeAssetPath(NormalizedPath);
	if (RecipeAssetPath.IsNull())
	{
		OutError = FString::Printf(TEXT("Recipe path invalido em config: '%s'."), *RecipePath);
		return false;
	}

	UObject* RecipeObject = RecipeAssetPath.TryLoad();
	UOmniAttributesRecipeDataAsset* RecipeAsset = Cast<UOmniAttributesRecipeDataAsset>(RecipeObject);
	if (!RecipeAsset)
	{
		OutError = FString::Printf(
			TEXT("Nao foi possivel carregar UOmniAttributesRecipeDataAsset em '%s'."),
			*RecipeAssetPath.ToString()
		);
		return false;
	}

	if (RecipeAsset->Attributes.Num() == 0)
	{
		OutError = FString::Printf(TEXT("Recipe '%s' nao contem atributos."), *GetNameSafe(RecipeAsset));
		return false;
	}

	TMap<FGameplayTag, FOmniAttributeValue> LoadedAttributes;
	FOmniStaminaRules LoadedStaminaRules = StaminaRules;
	for (const FOmniAttributeRecipeEntry& Entry : RecipeAsset->Attributes)
	{
		FString ValidationError;
		if (!Entry.IsValid(ValidationError))
		{
			OutError = FString::Printf(
				TEXT("Recipe '%s' contem entrada invalida para tag '%s': %s"),
				*GetNameSafe(RecipeAsset),
				*Entry.AttrTag.ToString(),
				*ValidationError
			);
			return false;
		}

		FOmniAttributeValue AttributeValue;
		AttributeValue.Tag = Entry.AttrTag;
		AttributeValue.Max = Entry.MaxValue;
		AttributeValue.Current = Entry.StartValue;
		LoadedAttributes.Add(Entry.AttrTag, AttributeValue);

		if (Entry.AttrTag == StaminaTag && Entry.bUseStaminaRules)
		{
			LoadedStaminaRules = Entry.StaminaRules;
		}
	}

	if (!LoadedAttributes.Contains(HpTag) || !LoadedAttributes.Contains(StaminaTag))
	{
		OutError = TEXT("Recipe precisa definir Game.Attr.HP e Game.Attr.Stamina.");
		return false;
	}

	FString RulesValidationError;
	if (!LoadedStaminaRules.IsValid(LoadedAttributes[StaminaTag].Max, RulesValidationError))
	{
		OutError = FString::Printf(TEXT("Stamina rules invalidas no recipe: %s"), *RulesValidationError);
		return false;
	}

	AttributesByTag = MoveTemp(LoadedAttributes);
	StaminaRules = LoadedStaminaRules;

	UE_LOG(
		LogOmniStatusSystem,
		Log,
		TEXT("[Omni][Attributes][Config] Recipe carregada | path=%s hp=%.1f/%.1f stamina=%.1f/%.1f"),
		*RecipeAssetPath.ToString(),
		AttributesByTag[HpTag].Current,
		AttributesByTag[HpTag].Max,
		AttributesByTag[StaminaTag].Current,
		AttributesByTag[StaminaTag].Max
	);

	return true;
}

void UOmniStatusSystem::ApplyFallbackRecipe()
{
	AttributesByTag.Reset();

	FOmniAttributeValue HpValue;
	HpValue.Tag = HpTag;
	HpValue.Max = 100.0f;
	HpValue.Current = 100.0f;
	AttributesByTag.Add(HpTag, HpValue);

	FOmniAttributeValue StaminaValue;
	StaminaValue.Tag = StaminaTag;
	StaminaValue.Max = 100.0f;
	StaminaValue.Current = 100.0f;
	AttributesByTag.Add(StaminaTag, StaminaValue);

	StaminaRules = FOmniStaminaRules();
}

void UOmniStatusSystem::ResolveOfficialTags()
{
	HpTag = FGameplayTag::RequestGameplayTag(OmniStatus::HpTagName, false);
	StaminaTag = FGameplayTag::RequestGameplayTag(OmniStatus::StaminaTagName, false);
	SprintingStateTag = FGameplayTag::RequestGameplayTag(OmniStatus::SprintingStateTagName, false);
	ExhaustedStateTag = FGameplayTag::RequestGameplayTag(OmniStatus::ExhaustedStateTagName, false);
	if (!ExhaustedStateTag.IsValid())
	{
		ExhaustedStateTag = FGameplayTag::RequestGameplayTag(TEXT("State.Exhausted"), false);
	}
}

void UOmniStatusSystem::InitializeSnapshot()
{
	if (const FOmniAttributeValue* HpValue = FindAttribute(HpTag))
	{
		Snapshot.HP = *HpValue;
	}
	else
	{
		Snapshot.HP = FOmniAttributeValue();
		Snapshot.HP.Tag = HpTag;
	}

	if (const FOmniAttributeValue* StaminaValue = FindAttribute(StaminaTag))
	{
		Snapshot.Stamina = *StaminaValue;
	}
	else
	{
		Snapshot.Stamina = FOmniAttributeValue();
		Snapshot.Stamina.Tag = StaminaTag;
	}

	Snapshot.bExhausted = bExhausted;
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

void UOmniStatusSystem::TickStamina(const float DeltaTime)
{
	FOmniAttributeValue* StaminaAttribute = FindAttributeMutable(StaminaTag);
	if (!StaminaAttribute)
	{
		return;
	}

	const bool bSprinting = SprintingStateTag.IsValid() && ContextTags.HasTagExact(SprintingStateTag);
	if (bSprinting)
	{
		StaminaAttribute->Current -= StaminaRules.DrainPerSecond * DeltaTime;
		TimeSinceLastStaminaDrain = 0.0f;
	}
	else
	{
		TimeSinceLastStaminaDrain += DeltaTime;
		if (TimeSinceLastStaminaDrain >= StaminaRules.RegenDelaySeconds)
		{
			StaminaAttribute->Current += StaminaRules.RegenPerSecond * DeltaTime;
		}
	}

	ClampAttribute(*StaminaAttribute);
	if (!bExhausted && StaminaAttribute->Current <= StaminaRules.ExhaustedThreshold)
	{
		SetExhausted(true);
	}
	else if (bExhausted && StaminaAttribute->Current >= StaminaRules.RecoverThreshold)
	{
		SetExhausted(false);
	}
}

FOmniAttributeValue* UOmniStatusSystem::FindAttributeMutable(const FGameplayTag AttributeTag)
{
	return AttributesByTag.Find(AttributeTag);
}

const FOmniAttributeValue* UOmniStatusSystem::FindAttribute(const FGameplayTag AttributeTag) const
{
	return AttributesByTag.Find(AttributeTag);
}

void UOmniStatusSystem::ClampAttribute(FOmniAttributeValue& Attribute) const
{
	Attribute.Max = FMath::Max(0.0f, Attribute.Max);
	Attribute.Current = FMath::Clamp(Attribute.Current, 0.0f, Attribute.Max);
}

void UOmniStatusSystem::SetExhausted(const bool bInExhausted)
{
	if (bExhausted == bInExhausted)
	{
		return;
	}

	bExhausted = bInExhausted;
	UpdateStateTags();

	if (!Registry.IsValid())
	{
		return;
	}

	if (bExhausted)
	{
		FOmniExhaustedEventSchema EventSchema;
		EventSchema.SourceSystem = OmniStatus::SystemId;
		Registry->BroadcastEvent(FOmniExhaustedEventSchema::ToMessage(EventSchema));
		UE_LOG(LogOmniStatusSystem, Log, TEXT("[Omni][Attributes] Game.State.Exhausted = True"));
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->LogWarning(OmniStatus::CategoryName, TEXT("Game.State.Exhausted = True"), OmniStatus::SourceName);
		}
	}
	else
	{
		FOmniExhaustedClearedEventSchema EventSchema;
		EventSchema.SourceSystem = OmniStatus::SystemId;
		Registry->BroadcastEvent(FOmniExhaustedClearedEventSchema::ToMessage(EventSchema));
		UE_LOG(LogOmniStatusSystem, Log, TEXT("[Omni][Attributes] Game.State.Exhausted = False"));
		if (DebugSubsystem.IsValid())
		{
			DebugSubsystem->LogEvent(OmniStatus::CategoryName, TEXT("Game.State.Exhausted = False"), OmniStatus::SourceName);
		}
	}
}

void UOmniStatusSystem::UpdateStateTags()
{
	StateTags = ContextTags;
	if (bExhausted && ExhaustedStateTag.IsValid())
	{
		StateTags.AddTag(ExhaustedStateTag);
	}
}

void UOmniStatusSystem::PublishTelemetry()
{
	if (!DebugSubsystem.IsValid())
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("Status.HP"), FString::Printf(TEXT("%.1f/%.1f"), Snapshot.HP.Current, Snapshot.HP.Max));
	DebugSubsystem->SetMetric(TEXT("Status.Stamina"), FString::Printf(TEXT("%.1f/%.1f"), Snapshot.Stamina.Current, Snapshot.Stamina.Max));
	DebugSubsystem->SetMetric(TEXT("Status.Exhausted"), Snapshot.bExhausted ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(
		TEXT("Status.Sprinting"),
		(SprintingStateTag.IsValid() && ContextTags.HasTagExact(SprintingStateTag)) ? TEXT("True") : TEXT("False")
	);
}
