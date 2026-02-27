#include "Forge/OmniForgeRunner.h"
#include "Forge/OmniForgeContext.h"

#include "Containers/StringConv.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/FileManager.h"
#include "Library/OmniActionLibrary.h"
#include "Library/OmniMovementLibrary.h"
#include "Library/OmniStatusLibrary.h"
#include "Manifest/OmniManifest.h"
#include "Manifest/OmniOfficialManifest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Profile/OmniActionProfile.h"
#include "Profile/OmniMovementProfile.h"
#include "Profile/OmniStatusProfile.h"
#include "Serialization/JsonWriter.h"
#include "Systems/ActionGate/OmniActionGateTypes.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniForge, Log, All);

namespace OmniForge
{
	static constexpr TCHAR SchemaName[] = TEXT("omni.forge.resolved_manifest.v1");
	static constexpr int32 ForgeVersion = 1;
	static constexpr TCHAR DefaultRoot[] = TEXT("/Game/Data");
	static constexpr TCHAR SavedFolder[] = TEXT("Omni");
	static constexpr TCHAR ResolvedManifestFile[] = TEXT("ResolvedManifest.json");
	static constexpr TCHAR ReportFile[] = TEXT("ForgeReport.md");
	static constexpr TCHAR DisplayResolvedManifestPath[] = TEXT("Saved/Omni/ResolvedManifest.json");
	static constexpr TCHAR DisplayReportPath[] = TEXT("Saved/Omni/ForgeReport.md");
	static constexpr TCHAR DisallowedActionPrefix[] = TEXT("Input.");

	enum class EProfileRuleKind : uint8
	{
		None,
		Action,
		Status,
		Movement
	};

	struct FExpectedProfileRule
	{
		FName SettingKey = NAME_None;
		EProfileRuleKind Kind = EProfileRuleKind::None;
		FString ProfileClassName;
		FString LibraryClassName;
	};

	static FString SeverityToString(const EOmniForgeErrorSeverity Severity)
	{
		return Severity == EOmniForgeErrorSeverity::Error ? TEXT("ERROR") : TEXT("WARNING");
	}

	static FString NormalizeSlashes(const FString& Input)
	{
		FString Result = Input;
		Result.TrimStartAndEndInline();
		Result.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Result.Contains(TEXT("//")))
		{
			Result.ReplaceInline(TEXT("//"), TEXT("/"));
		}
		return Result;
	}

	static FString NormalizeRootPath(const FString& InRoot)
	{
		FString Root = NormalizeSlashes(InRoot);
		if (Root.IsEmpty())
		{
			Root = DefaultRoot;
		}

		if (!Root.StartsWith(TEXT("/")))
		{
			Root = TEXT("/") + Root;
		}

		if (!Root.StartsWith(TEXT("/Game")))
		{
			Root = TEXT("/Game/") + Root.RightChop(1);
		}

		while (Root.EndsWith(TEXT("/")))
		{
			Root.LeftChopInline(1, EAllowShrinking::No);
		}

		if (Root.IsEmpty())
		{
			Root = DefaultRoot;
		}

		return Root;
	}

	static FString NormalizeObjectPath(const FString& InObjectPath)
	{
		return NormalizeSlashes(InObjectPath);
	}

	static FName NormalizeNameToken(const FName InName)
	{
		if (InName == NAME_None)
		{
			return NAME_None;
		}

		FString Value = InName.ToString();
		Value.TrimStartAndEndInline();
		return Value.IsEmpty() ? NAME_None : FName(*Value);
	}

	static FString BuildManifestSource(const FOmniForgeInput& Input)
	{
		if (!Input.ManifestAssetPath.IsNull())
		{
			return Input.ManifestAssetPath.ToString();
		}
		if (!Input.ManifestClassPath.IsNull())
		{
			return Input.ManifestClassPath.ToString();
		}
		return UOmniOfficialManifest::StaticClass()->GetPathName();
	}

	static FExpectedProfileRule GetExpectedProfileRule(const FName SystemId)
	{
		if (SystemId == TEXT("ActionGate"))
		{
			FExpectedProfileRule Rule;
			Rule.SettingKey = TEXT("ActionProfileAssetPath");
			Rule.Kind = EProfileRuleKind::Action;
			Rule.ProfileClassName = UOmniActionProfile::StaticClass()->GetName();
			Rule.LibraryClassName = UOmniActionLibrary::StaticClass()->GetName();
			return Rule;
		}
		if (SystemId == TEXT("Status"))
		{
			FExpectedProfileRule Rule;
			Rule.SettingKey = TEXT("StatusProfileAssetPath");
			Rule.Kind = EProfileRuleKind::Status;
			Rule.ProfileClassName = UOmniStatusProfile::StaticClass()->GetName();
			Rule.LibraryClassName = UOmniStatusLibrary::StaticClass()->GetName();
			return Rule;
		}
		if (SystemId == TEXT("Movement"))
		{
			FExpectedProfileRule Rule;
			Rule.SettingKey = TEXT("MovementProfileAssetPath");
			Rule.Kind = EProfileRuleKind::Movement;
			Rule.ProfileClassName = UOmniMovementProfile::StaticClass()->GetName();
			Rule.LibraryClassName = UOmniMovementLibrary::StaticClass()->GetName();
			return Rule;
		}

		return {};
	}

	static FString BuildIssueLocation(const FName SystemId, const FName SettingKey)
	{
		if (SettingKey == NAME_None)
		{
			return FString::Printf(TEXT("SystemId=%s"), *SystemId.ToString());
		}

		return FString::Printf(TEXT("SystemId=%s Setting=%s"), *SystemId.ToString(), *SettingKey.ToString());
	}

	static bool TryResolveManifest(const FOmniForgeInput& Input, UOmniManifest*& OutManifest, FOmniForgeReport& Report)
	{
		OutManifest = nullptr;
		Report.ManifestSource = BuildManifestSource(Input);

		if (!Input.ManifestAssetPath.IsNull())
		{
			UObject* LoadedAsset = Input.ManifestAssetPath.TryLoad();
			if (!LoadedAsset)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E010_MISSING_ASSET"),
					FString::Printf(TEXT("Manifest asset not found: %s"), *Input.ManifestAssetPath.ToString()),
					TEXT("ManifestAssetPath"),
					TEXT("Point ManifestAssetPath to an existing UOmniManifest asset.")
				);
				return false;
			}

			UOmniManifest* AssetManifest = Cast<UOmniManifest>(LoadedAsset);
			if (!AssetManifest)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
					FString::Printf(
						TEXT("Manifest asset type mismatch: %s is '%s' (expected UOmniManifest)."),
						*Input.ManifestAssetPath.ToString(),
						*LoadedAsset->GetClass()->GetPathName()
					),
					TEXT("ManifestAssetPath"),
					TEXT("Use a UOmniManifest-derived asset as source manifest.")
				);
				return false;
			}

			OutManifest = AssetManifest;
			return true;
		}

		FSoftClassPath ManifestClassPath = Input.ManifestClassPath;
		if (ManifestClassPath.IsNull())
		{
			ManifestClassPath = FSoftClassPath(UOmniOfficialManifest::StaticClass());
		}

		UClass* LoadedClass = ManifestClassPath.TryLoadClass<UOmniManifest>();
		if (!LoadedClass || !LoadedClass->IsChildOf(UOmniManifest::StaticClass()))
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
				FString::Printf(TEXT("Manifest class path is invalid: %s"), *ManifestClassPath.ToString()),
				TEXT("ManifestClassPath"),
				TEXT("Set ManifestClassPath to a valid UOmniManifest-derived class.")
			);
			return false;
		}

		OutManifest = NewObject<UOmniManifest>(GetTransientPackage(), LoadedClass, NAME_None, RF_Transient);
		if (!OutManifest)
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E099_INTERNAL"),
				FString::Printf(TEXT("Failed to instantiate manifest class: %s"), *ManifestClassPath.ToString()),
				TEXT("ManifestClassPath"),
				TEXT("Verify class constructor and dependencies.")
			);
			return false;
		}

		return true;
	}

	static void NormalizeManifest(const UOmniManifest* Manifest, const FOmniForgeInput& Input, FOmniForgeNormalized& OutNormalized)
	{
		OutNormalized = FOmniForgeNormalized();
		OutNormalized.GenerationRoot = NormalizeRootPath(Input.GenerationRoot);
		OutNormalized.Namespace = Manifest ? Manifest->Namespace.ToString().TrimStartAndEnd() : FString();
		OutNormalized.BuildVersion = Manifest ? Manifest->BuildVersion : 0;

		if (!Manifest)
		{
			return;
		}

		for (const FOmniSystemManifestEntry& Entry : Manifest->Systems)
		{
			if (!Entry.bEnabled)
			{
				continue;
			}

			FOmniForgeNormalizedSystem& System = OutNormalized.Systems.AddDefaulted_GetRef();
			System.SystemId = NormalizeNameToken(Entry.SystemId);
			System.SystemClassPath = Entry.SystemClass.ToSoftObjectPath().ToString();

			for (const FName Dependency : Entry.Dependencies)
			{
				const FName NormalizedDependency = NormalizeNameToken(Dependency);
				if (NormalizedDependency == NAME_None)
				{
					continue;
				}
				System.Dependencies.AddUnique(NormalizedDependency);
			}
			System.Dependencies.Sort(FNameLexicalLess());

			TArray<FName> SettingKeys = Entry.GetSettingKeysSnapshot();
			SettingKeys.Sort(FNameLexicalLess());
			for (const FName Key : SettingKeys)
			{
				FString Value;
				if (!Entry.TryGetSetting(Key, Value))
				{
					continue;
				}
				System.SetSetting(Key, NormalizeObjectPath(Value));
			}
		}

		OutNormalized.Systems.Sort(
			[](const FOmniForgeNormalizedSystem& Left, const FOmniForgeNormalizedSystem& Right)
			{
				return FNameLexicalLess()(Left.SystemId, Right.SystemId);
			}
		);
	}

	static bool BuildNormalizedInputJsonString(const FOmniForgeNormalized& Normalized, FString& OutJson)
	{
		OutJson.Reset();
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);

		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("namespace"), Normalized.Namespace);
		Writer->WriteValue(TEXT("buildVersion"), Normalized.BuildVersion);

		Writer->WriteArrayStart(TEXT("systems"));
		for (const FOmniForgeNormalizedSystem& System : Normalized.Systems)
		{
			Writer->WriteObjectStart();
			Writer->WriteValue(TEXT("systemId"), System.SystemId.ToString());
			Writer->WriteValue(TEXT("systemClassPath"), System.SystemClassPath);

			Writer->WriteArrayStart(TEXT("dependencies"));
			for (const FName Dependency : System.Dependencies)
			{
				Writer->WriteValue(Dependency.ToString());
			}
			Writer->WriteArrayEnd();

			TArray<FName> SettingKeys;
			System.GetSettingKeys(SettingKeys);
			SettingKeys.Sort(FNameLexicalLess());

			Writer->WriteObjectStart(TEXT("settings"));
			for (const FName SettingKey : SettingKeys)
			{
				const FString* SettingValue = nullptr;
				System.TryGetSettingPtr(SettingKey, SettingValue);
				Writer->WriteValue(SettingKey.ToString(), SettingValue ? *SettingValue : FString());
			}
			Writer->WriteObjectEnd();

			Writer->WriteObjectEnd();
		}
		Writer->WriteArrayEnd();
		Writer->WriteObjectEnd();

		return Writer->Close();
	}

	static bool ComputeNormalizedInputHash(const FOmniForgeNormalized& Normalized, FString& OutHash)
	{
		OutHash.Reset();

		FString CanonicalInputJson;
		if (!BuildNormalizedInputJsonString(Normalized, CanonicalInputJson))
		{
			return false;
		}

		FTCHARToUTF8 Utf8Data(*CanonicalInputJson);
		if (Utf8Data.Length() < 0)
		{
			return false;
		}
		if (Utf8Data.Length() > MAX_uint32)
		{
			return false;
		}

		FSHA256Signature Signature;
		const bool bOk = FPlatformMisc::GetSHA256Signature(
			reinterpret_cast<const uint8*>(Utf8Data.Get()),
			static_cast<uint32>(Utf8Data.Length()),
			Signature
		);
		if (!bOk)
		{
			return false;
		}

		OutHash = Signature.ToString();
		return !OutHash.IsEmpty();
	}

	static bool BuildInitializationOrder(
		const TMap<FName, const FOmniForgeNormalizedSystem*>& SystemsById,
		TArray<FName>& OutOrder,
		TArray<FName>& OutCycleCandidates
	)
	{
		OutOrder.Reset();
		OutCycleCandidates.Reset();

		TMap<FName, int32> InDegree;
		TMap<FName, TArray<FName>> OutEdges;

		for (const TPair<FName, const FOmniForgeNormalizedSystem*>& Pair : SystemsById)
		{
			InDegree.Add(Pair.Key, 0);
		}

		for (const TPair<FName, const FOmniForgeNormalizedSystem*>& Pair : SystemsById)
		{
			const FName SystemId = Pair.Key;
			const FOmniForgeNormalizedSystem& System = *Pair.Value;

			for (const FName Dependency : System.Dependencies)
			{
				if (Dependency == NAME_None || Dependency == SystemId)
				{
					continue;
				}
				if (!SystemsById.Contains(Dependency))
				{
					continue;
				}

				OutEdges.FindOrAdd(Dependency).AddUnique(SystemId);
				InDegree[SystemId] = InDegree[SystemId] + 1;
			}
		}

		TArray<FName> Ready;
		for (const TPair<FName, int32>& Pair : InDegree)
		{
			if (Pair.Value == 0)
			{
				Ready.Add(Pair.Key);
			}
		}
		Ready.Sort(FNameLexicalLess());

		while (Ready.Num() > 0)
		{
			const FName Current = Ready[0];
			Ready.RemoveAt(0, 1, EAllowShrinking::No);
			OutOrder.Add(Current);

			if (const TArray<FName>* Dependents = OutEdges.Find(Current))
			{
				for (const FName Dependent : *Dependents)
				{
					int32& Degree = InDegree.FindChecked(Dependent);
					Degree = FMath::Max(0, Degree - 1);
					if (Degree == 0)
					{
						Ready.AddUnique(Dependent);
					}
				}
			}
			Ready.Sort(FNameLexicalLess());
		}

		if (OutOrder.Num() == SystemsById.Num())
		{
			return true;
		}

		for (const TPair<FName, int32>& Pair : InDegree)
		{
			if (Pair.Value > 0)
			{
				OutCycleCandidates.Add(Pair.Key);
			}
		}
		OutCycleCandidates.Sort(FNameLexicalLess());
		return false;
	}

	static bool IsDisallowedActionId(const FName ActionId)
	{
		return NormalizeNameToken(ActionId).ToString().StartsWith(DisallowedActionPrefix, ESearchCase::CaseSensitive);
	}

	static TArray<FString> ToSortedTagNames(const FGameplayTagContainer& Container)
	{
		TArray<FString> Result;
		for (const FGameplayTag& Tag : Container)
		{
			if (Tag.IsValid())
			{
				Result.Add(Tag.ToString());
			}
		}
		Result.Sort();
		return Result;
	}

	static FOmniForgeResolvedAction ToResolvedAction(const FOmniActionDefinition& Definition)
	{
		FOmniForgeResolvedAction Action;
		Action.ActionId = NormalizeNameToken(Definition.ActionId);
		Action.bEnabled = Definition.bEnabled;

		switch (Definition.Policy)
		{
		case EOmniActionPolicy::DenyIfActive:
			Action.Policy = TEXT("DenyIfActive");
			break;
		case EOmniActionPolicy::SucceedIfActive:
			Action.Policy = TEXT("SucceedIfActive");
			break;
		case EOmniActionPolicy::RestartIfActive:
			Action.Policy = TEXT("RestartIfActive");
			break;
		default:
			Action.Policy = TEXT("Unknown");
			break;
		}

		Action.BlockedBy = ToSortedTagNames(Definition.BlockedBy);
		for (const FName CancelId : Definition.Cancels)
		{
			const FName NormalizedCancel = NormalizeNameToken(CancelId);
			if (NormalizedCancel != NAME_None)
			{
				Action.Cancels.AddUnique(NormalizedCancel);
			}
		}
		Action.Cancels.Sort(FNameLexicalLess());
		Action.AppliesLocks = ToSortedTagNames(Definition.AppliesLocks);
		return Action;
	}

	static bool ValidateProfileAndLibraryForSystem(
		const FOmniForgeNormalizedSystem& System,
		const FExpectedProfileRule& Rule,
		const bool bRequireContentAssets,
		FOmniForgeResolvedProfile& OutProfile,
		TArray<FOmniForgeResolvedAction>& OutActionDefinitions,
		FOmniForgeReport& Report
	)
	{
		OutActionDefinitions.Reset();
		OutProfile = FOmniForgeResolvedProfile();
		OutProfile.SystemId = System.SystemId;
		OutProfile.SettingKey = Rule.SettingKey;

		const FString* RawPath = nullptr;
		System.TryGetSettingPtr(Rule.SettingKey, RawPath);
		if (!RawPath || RawPath->IsEmpty())
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E003_MISSING_SETTING"),
				FString::Printf(
					TEXT("Missing required setting '%s' for system '%s'."),
					*Rule.SettingKey.ToString(),
					*System.SystemId.ToString()
				),
				BuildIssueLocation(System.SystemId, Rule.SettingKey),
				FString::Printf(
					TEXT("Set '%s' to a valid %s asset path."),
					*Rule.SettingKey.ToString(),
					*Rule.ProfileClassName
				)
			);
			return false;
		}

		const FString ProfilePath = NormalizeObjectPath(*RawPath);
		OutProfile.ProfileAssetPath = ProfilePath;

		if (!ProfilePath.StartsWith(TEXT("/Game/")))
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E004_INVALID_PATH"),
				FString::Printf(
					TEXT("Invalid profile asset path '%s' for system '%s'."),
					*ProfilePath,
					*System.SystemId.ToString()
				),
				BuildIssueLocation(System.SystemId, Rule.SettingKey),
				TEXT("Use a valid object path starting with /Game/.")
			);
			return false;
		}

		if (!bRequireContentAssets)
		{
			return true;
		}

		const FSoftObjectPath SoftPath(ProfilePath);
		if (SoftPath.IsNull())
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E004_INVALID_PATH"),
				FString::Printf(
					TEXT("Malformed profile object path '%s' for system '%s'."),
					*ProfilePath,
					*System.SystemId.ToString()
				),
				BuildIssueLocation(System.SystemId, Rule.SettingKey),
				TEXT("Fix the object path format: /Game/Folder/Asset.Asset.")
			);
			return false;
		}

		UObject* LoadedProfileObject = SoftPath.TryLoad();
		if (!LoadedProfileObject)
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E010_MISSING_ASSET"),
				FString::Printf(
					TEXT("Profile asset not found for system '%s': %s"),
					*System.SystemId.ToString(),
					*ProfilePath
				),
				BuildIssueLocation(System.SystemId, Rule.SettingKey),
				FString::Printf(
					TEXT("Create a %s asset at this path and assign its %s."),
					*Rule.ProfileClassName,
					*Rule.LibraryClassName
				)
			);
			return false;
		}

		OutProfile.ProfileClassPath = LoadedProfileObject->GetClass()->GetPathName();

		if (Rule.Kind == EProfileRuleKind::Action)
		{
			UOmniActionProfile* ActionProfile = Cast<UOmniActionProfile>(LoadedProfileObject);
			if (!ActionProfile)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
					FString::Printf(
						TEXT("Profile '%s' is '%s' (expected %s)."),
						*ProfilePath,
						*LoadedProfileObject->GetClass()->GetPathName(),
						*Rule.ProfileClassName
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Use the correct ActionProfile asset class.")
				);
				return false;
			}

			const FSoftObjectPath LibraryPath = ActionProfile->ActionLibrary.ToSoftObjectPath();
			OutProfile.LibraryAssetPath = LibraryPath.ToString();
			if (LibraryPath.IsNull())
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E012_NULL_LIBRARY"),
					FString::Printf(TEXT("Profile '%s' has null ActionLibrary."), *GetNameSafe(ActionProfile)),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Assign a UOmniActionLibrary to ActionLibrary.")
				);
				return false;
			}

			UObject* LoadedLibraryObject = LibraryPath.TryLoad();
			if (!LoadedLibraryObject)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E010_MISSING_ASSET"),
					FString::Printf(
						TEXT("ActionLibrary not found for profile '%s': %s"),
						*GetNameSafe(ActionProfile),
						*LibraryPath.ToString()
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Create the library asset and assign it in the profile.")
				);
				return false;
			}

			UOmniActionLibrary* ActionLibrary = Cast<UOmniActionLibrary>(LoadedLibraryObject);
			if (!ActionLibrary)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
					FString::Printf(
						TEXT("ActionLibrary '%s' is '%s' (expected %s)."),
						*LibraryPath.ToString(),
						*LoadedLibraryObject->GetClass()->GetPathName(),
						*Rule.LibraryClassName
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Assign a UOmniActionLibrary-derived asset.")
				);
				return false;
			}
			OutProfile.LibraryClassPath = ActionLibrary->GetClass()->GetPathName();

			TArray<FOmniActionDefinition> Definitions;
			ActionProfile->ResolveDefinitions(Definitions);
			if (Definitions.Num() == 0)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E015_INVALID_PROFILE_CONFIG"),
					FString::Printf(TEXT("Action profile '%s' resolved zero definitions."), *GetNameSafe(ActionProfile)),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Populate ActionLibrary definitions or OverrideDefinitions.")
				);
				return false;
			}

			TSet<FName> SeenActionIds;
			for (const FOmniActionDefinition& Definition : Definitions)
			{
				const FName NormalizedActionId = NormalizeNameToken(Definition.ActionId);
				if (NormalizedActionId == NAME_None)
				{
					Report.AddWarning(
						TEXT("OMNI_FORGE_W001_EMPTY_ACTIONID"),
						FString::Printf(
							TEXT("Action profile '%s' contains an entry with empty ActionId; entry ignored."),
							*GetNameSafe(ActionProfile)
						),
						BuildIssueLocation(System.SystemId, Rule.SettingKey),
						TEXT("Set ActionId to a valid gameplay/system id (example: Movement.Sprint).")
					);
					continue;
				}

				if (IsDisallowedActionId(NormalizedActionId))
				{
					Report.AddError(
						TEXT("OMNI_FORGE_E020_INVALID_ACTIONID_PREFIX"),
						FString::Printf(
							TEXT("ActionId '%s' is invalid in profile '%s'."),
							*NormalizedActionId.ToString(),
							*GetNameSafe(ActionProfile)
						),
						BuildIssueLocation(System.SystemId, Rule.SettingKey),
						TEXT("ActionId must be gameplay/system-level (example: Movement.Sprint), not Input.*")
					);
					continue;
				}

				if (SeenActionIds.Contains(NormalizedActionId))
				{
					Report.AddError(
						TEXT("OMNI_FORGE_E002_DUPLICATE_ACTIONID"),
						FString::Printf(
							TEXT("Duplicate ActionId '%s' found after profile resolution."),
							*NormalizedActionId.ToString()
						),
						BuildIssueLocation(System.SystemId, Rule.SettingKey),
						TEXT("Keep a single canonical ActionId definition.")
					);
					continue;
				}
				SeenActionIds.Add(NormalizedActionId);

				OutActionDefinitions.Add(ToResolvedAction(Definition));
			}

			return OutActionDefinitions.Num() > 0;
		}

		if (Rule.Kind == EProfileRuleKind::Status)
		{
			UOmniStatusProfile* StatusProfile = Cast<UOmniStatusProfile>(LoadedProfileObject);
			if (!StatusProfile)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
					FString::Printf(
						TEXT("Profile '%s' is '%s' (expected %s)."),
						*ProfilePath,
						*LoadedProfileObject->GetClass()->GetPathName(),
						*Rule.ProfileClassName
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Use the correct StatusProfile asset class.")
				);
				return false;
			}

			const FSoftObjectPath LibraryPath = StatusProfile->StatusLibrary.ToSoftObjectPath();
			OutProfile.LibraryAssetPath = LibraryPath.ToString();
			if (LibraryPath.IsNull())
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E012_NULL_LIBRARY"),
					FString::Printf(TEXT("Profile '%s' has null StatusLibrary."), *GetNameSafe(StatusProfile)),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Assign a UOmniStatusLibrary to StatusLibrary.")
				);
				return false;
			}

			UObject* LoadedLibraryObject = LibraryPath.TryLoad();
			if (!LoadedLibraryObject)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E010_MISSING_ASSET"),
					FString::Printf(
						TEXT("StatusLibrary not found for profile '%s': %s"),
						*GetNameSafe(StatusProfile),
						*LibraryPath.ToString()
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Create the library asset and assign it in the profile.")
				);
				return false;
			}

			UOmniStatusLibrary* StatusLibrary = Cast<UOmniStatusLibrary>(LoadedLibraryObject);
			if (!StatusLibrary)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
					FString::Printf(
						TEXT("StatusLibrary '%s' is '%s' (expected %s)."),
						*LibraryPath.ToString(),
						*LoadedLibraryObject->GetClass()->GetPathName(),
						*Rule.LibraryClassName
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Assign a UOmniStatusLibrary-derived asset.")
				);
				return false;
			}
			OutProfile.LibraryClassPath = StatusLibrary->GetClass()->GetPathName();

			FOmniStatusSettings ResolvedSettings;
			if (!StatusProfile->ResolveSettings(ResolvedSettings))
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E015_INVALID_PROFILE_CONFIG"),
					FString::Printf(TEXT("Status profile '%s' has no valid resolved settings."), *GetNameSafe(StatusProfile)),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Configure StatusLibrary and/or valid overrides.")
				);
				return false;
			}

			return true;
		}

		if (Rule.Kind == EProfileRuleKind::Movement)
		{
			UOmniMovementProfile* MovementProfile = Cast<UOmniMovementProfile>(LoadedProfileObject);
			if (!MovementProfile)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
					FString::Printf(
						TEXT("Profile '%s' is '%s' (expected %s)."),
						*ProfilePath,
						*LoadedProfileObject->GetClass()->GetPathName(),
						*Rule.ProfileClassName
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Use the correct MovementProfile asset class.")
				);
				return false;
			}

			const FSoftObjectPath LibraryPath = MovementProfile->MovementLibrary.ToSoftObjectPath();
			OutProfile.LibraryAssetPath = LibraryPath.ToString();
			if (LibraryPath.IsNull())
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E012_NULL_LIBRARY"),
					FString::Printf(TEXT("Profile '%s' has null MovementLibrary."), *GetNameSafe(MovementProfile)),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Assign a UOmniMovementLibrary to MovementLibrary.")
				);
				return false;
			}

			UObject* LoadedLibraryObject = LibraryPath.TryLoad();
			if (!LoadedLibraryObject)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E010_MISSING_ASSET"),
					FString::Printf(
						TEXT("MovementLibrary not found for profile '%s': %s"),
						*GetNameSafe(MovementProfile),
						*LibraryPath.ToString()
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Create the library asset and assign it in the profile.")
				);
				return false;
			}

			UOmniMovementLibrary* MovementLibrary = Cast<UOmniMovementLibrary>(LoadedLibraryObject);
			if (!MovementLibrary)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E011_TYPE_MISMATCH"),
					FString::Printf(
						TEXT("MovementLibrary '%s' is '%s' (expected %s)."),
						*LibraryPath.ToString(),
						*LoadedLibraryObject->GetClass()->GetPathName(),
						*Rule.LibraryClassName
					),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Assign a UOmniMovementLibrary-derived asset.")
				);
				return false;
			}
			OutProfile.LibraryClassPath = MovementLibrary->GetClass()->GetPathName();

			FOmniMovementSettings ResolvedSettings;
			if (!MovementProfile->ResolveSettings(ResolvedSettings))
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E015_INVALID_PROFILE_CONFIG"),
					FString::Printf(TEXT("Movement profile '%s' has no valid resolved settings."), *GetNameSafe(MovementProfile)),
					BuildIssueLocation(System.SystemId, Rule.SettingKey),
					TEXT("Configure MovementLibrary and/or valid overrides.")
				);
				return false;
			}

			return true;
		}

		return true;
	}

	static void ValidateAndResolve(
		const FOmniForgeNormalized& Normalized,
		const bool bRequireContentAssets,
		TArray<FName>& OutInitializationOrder,
		TArray<FOmniForgeResolvedProfile>& OutProfiles,
		TArray<FOmniForgeResolvedAction>& OutActionDefinitions,
		FOmniForgeReport& Report
	)
	{
		OutInitializationOrder.Reset();
		OutProfiles.Reset();
		OutActionDefinitions.Reset();

		if (Normalized.Systems.Num() == 0)
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E030_EMPTY_MANIFEST"),
				TEXT("Manifest contains zero enabled systems."),
				TEXT("Manifest"),
				TEXT("Add at least one enabled system entry to the manifest.")
			);
			return;
		}

		TMap<FName, const FOmniForgeNormalizedSystem*> SystemsById;
		for (const FOmniForgeNormalizedSystem& System : Normalized.Systems)
		{
			if (System.SystemId == NAME_None)
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E003_MISSING_SYSTEMID"),
					TEXT("A system entry has empty SystemId."),
					TEXT("Manifest.Systems"),
					TEXT("Define a non-empty SystemId for every enabled system.")
				);
				continue;
			}

			if (SystemsById.Contains(System.SystemId))
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E001_DUPLICATE_SYSTEMID"),
					FString::Printf(TEXT("Duplicate SystemId '%s'."), *System.SystemId.ToString()),
					TEXT("Manifest.Systems"),
					TEXT("Ensure each enabled system has a unique SystemId.")
				);
				continue;
			}

			if (System.SystemClassPath.IsEmpty())
			{
				Report.AddError(
					TEXT("OMNI_FORGE_E022_MISSING_SYSTEMCLASS"),
					FString::Printf(TEXT("System '%s' has empty SystemClass."), *System.SystemId.ToString()),
					BuildIssueLocation(System.SystemId, NAME_None),
					TEXT("Set SystemClass to a valid UOmniRuntimeSystem-derived class.")
				);
			}

			SystemsById.Add(System.SystemId, &System);
		}

		for (const FOmniForgeNormalizedSystem& System : Normalized.Systems)
		{
			for (const FName Dependency : System.Dependencies)
			{
				if (Dependency == NAME_None)
				{
					continue;
				}
				if (Dependency == System.SystemId)
				{
					Report.AddError(
						TEXT("OMNI_FORGE_E005_MISSING_DEPENDENCY"),
						FString::Printf(TEXT("System '%s' cannot depend on itself."), *System.SystemId.ToString()),
						BuildIssueLocation(System.SystemId, NAME_None),
						TEXT("Remove the self dependency.")
					);
					continue;
				}
				if (!SystemsById.Contains(Dependency))
				{
					Report.AddError(
						TEXT("OMNI_FORGE_E005_MISSING_DEPENDENCY"),
						FString::Printf(
							TEXT("System '%s' depends on missing system '%s'."),
							*System.SystemId.ToString(),
							*Dependency.ToString()
						),
						BuildIssueLocation(System.SystemId, NAME_None),
						TEXT("Add the dependency system or remove this dependency.")
					);
				}
			}
		}

		TArray<FName> CycleCandidates;
		if (!BuildInitializationOrder(SystemsById, OutInitializationOrder, CycleCandidates))
		{
			Report.AddError(
				TEXT("OMNI_FORGE_E006_DEPENDENCY_CYCLE"),
				FString::Printf(
					TEXT("Dependency cycle detected among systems: %s"),
					*FString::JoinBy(
						CycleCandidates,
						TEXT(", "),
						[](const FName Name)
						{
							return Name.ToString();
						}
					)
				),
				TEXT("Manifest.Systems.Dependencies"),
				TEXT("Break the dependency cycle so initialization can be topologically ordered.")
			);
		}

		for (const FOmniForgeNormalizedSystem& System : Normalized.Systems)
		{
			const FExpectedProfileRule Rule = GetExpectedProfileRule(System.SystemId);
			if (Rule.Kind == EProfileRuleKind::None)
			{
				continue;
			}

			FOmniForgeResolvedProfile Profile;
			TArray<FOmniForgeResolvedAction> Actions;
			const bool bProfileOk = ValidateProfileAndLibraryForSystem(
				System,
				Rule,
				bRequireContentAssets,
				Profile,
				Actions,
				Report
			);
			if (!bProfileOk)
			{
				continue;
			}

			OutProfiles.Add(Profile);
			for (FOmniForgeResolvedAction& Action : Actions)
			{
				OutActionDefinitions.Add(MoveTemp(Action));
			}
		}

		OutProfiles.Sort(
			[](const FOmniForgeResolvedProfile& Left, const FOmniForgeResolvedProfile& Right)
			{
				if (Left.SystemId != Right.SystemId)
				{
					return FNameLexicalLess()(Left.SystemId, Right.SystemId);
				}
				return FNameLexicalLess()(Left.SettingKey, Right.SettingKey);
			}
		);
		OutActionDefinitions.Sort(
			[](const FOmniForgeResolvedAction& Left, const FOmniForgeResolvedAction& Right)
			{
				return FNameLexicalLess()(Left.ActionId, Right.ActionId);
			}
		);
	}

	static void BuildResolved(
		const FOmniForgeNormalized& Normalized,
		const TArray<FName>& InitializationOrder,
		const TArray<FOmniForgeResolvedProfile>& Profiles,
		const TArray<FOmniForgeResolvedAction>& ActionDefinitions,
		const FString& InputHash,
		FOmniForgeResolved& OutResolved
	)
	{
		OutResolved = FOmniForgeResolved();
		OutResolved.ForgeVersion = ForgeVersion;
		OutResolved.InputHash = InputHash;
		OutResolved.GenerationRoot = Normalized.GenerationRoot;
		OutResolved.Namespace = Normalized.Namespace;
		OutResolved.BuildVersion = Normalized.BuildVersion;
		OutResolved.InitializationOrder = InitializationOrder;
		OutResolved.Profiles = Profiles;
		OutResolved.ActionDefinitions = ActionDefinitions;

		for (const FOmniForgeNormalizedSystem& System : Normalized.Systems)
		{
			FOmniForgeResolvedSystem& ResolvedSystem = OutResolved.Systems.AddDefaulted_GetRef();
			ResolvedSystem.SystemId = System.SystemId;
			ResolvedSystem.SystemClassPath = System.SystemClassPath;
			ResolvedSystem.Dependencies = System.Dependencies;
			ResolvedSystem.Dependencies.Sort(FNameLexicalLess());
		}

		OutResolved.Systems.Sort(
			[](const FOmniForgeResolvedSystem& Left, const FOmniForgeResolvedSystem& Right)
			{
				return FNameLexicalLess()(Left.SystemId, Right.SystemId);
			}
		);

		OutResolved.SystemsCount = OutResolved.Systems.Num();
		OutResolved.ActionsCount = OutResolved.ActionDefinitions.Num();
	}

	static bool BuildResolvedJsonString(const FOmniForgeResolved& Resolved, FString& OutJson)
	{
		OutJson.Reset();
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);

		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("forgeVersion"), Resolved.ForgeVersion);
		Writer->WriteValue(TEXT("inputHash"), Resolved.InputHash);
		Writer->WriteValue(TEXT("systemsCount"), Resolved.SystemsCount);
		Writer->WriteValue(TEXT("actionsCount"), Resolved.ActionsCount);
		Writer->WriteNull(TEXT("generatedAt"));
		Writer->WriteValue(TEXT("schema"), SchemaName);
		Writer->WriteValue(TEXT("generationRoot"), Resolved.GenerationRoot);
		Writer->WriteValue(TEXT("namespace"), Resolved.Namespace);
		Writer->WriteValue(TEXT("buildVersion"), Resolved.BuildVersion);

		Writer->WriteArrayStart(TEXT("systems"));
		for (const FOmniForgeResolvedSystem& System : Resolved.Systems)
		{
			Writer->WriteObjectStart();
			Writer->WriteValue(TEXT("systemId"), System.SystemId.ToString());
			Writer->WriteValue(TEXT("systemClassPath"), System.SystemClassPath);
			Writer->WriteArrayStart(TEXT("dependencies"));
			for (const FName Dependency : System.Dependencies)
			{
				Writer->WriteValue(Dependency.ToString());
			}
			Writer->WriteArrayEnd();
			Writer->WriteObjectEnd();
		}
		Writer->WriteArrayEnd();

		Writer->WriteArrayStart(TEXT("initializationOrder"));
		for (const FName SystemId : Resolved.InitializationOrder)
		{
			Writer->WriteValue(SystemId.ToString());
		}
		Writer->WriteArrayEnd();

		Writer->WriteArrayStart(TEXT("profiles"));
		for (const FOmniForgeResolvedProfile& Profile : Resolved.Profiles)
		{
			Writer->WriteObjectStart();
			Writer->WriteValue(TEXT("systemId"), Profile.SystemId.ToString());
			Writer->WriteValue(TEXT("settingKey"), Profile.SettingKey.ToString());
			Writer->WriteValue(TEXT("profileAssetPath"), Profile.ProfileAssetPath);
			Writer->WriteValue(TEXT("profileClassPath"), Profile.ProfileClassPath);
			Writer->WriteValue(TEXT("libraryAssetPath"), Profile.LibraryAssetPath);
			Writer->WriteValue(TEXT("libraryClassPath"), Profile.LibraryClassPath);
			Writer->WriteObjectEnd();
		}
		Writer->WriteArrayEnd();

		Writer->WriteArrayStart(TEXT("actionDefinitions"));
		for (const FOmniForgeResolvedAction& Action : Resolved.ActionDefinitions)
		{
			Writer->WriteObjectStart();
			Writer->WriteValue(TEXT("actionId"), Action.ActionId.ToString());
			Writer->WriteValue(TEXT("enabled"), Action.bEnabled);
			Writer->WriteValue(TEXT("policy"), Action.Policy);

			Writer->WriteArrayStart(TEXT("blockedBy"));
			for (const FString& Tag : Action.BlockedBy)
			{
				Writer->WriteValue(Tag);
			}
			Writer->WriteArrayEnd();

			Writer->WriteArrayStart(TEXT("cancels"));
			for (const FName CancelAction : Action.Cancels)
			{
				Writer->WriteValue(CancelAction.ToString());
			}
			Writer->WriteArrayEnd();

			Writer->WriteArrayStart(TEXT("appliesLocks"));
			for (const FString& Lock : Action.AppliesLocks)
			{
				Writer->WriteValue(Lock);
			}
			Writer->WriteArrayEnd();

			Writer->WriteObjectEnd();
		}
		Writer->WriteArrayEnd();

		Writer->WriteObjectEnd();
		return Writer->Close();
	}

	static FString BuildReportMarkdown(const FOmniForgeReport& Report)
	{
		TArray<FString> Lines;
		Lines.Reserve(64 + Report.Errors.Num() * 2);

		Lines.Add(TEXT("# OMNI Forge Report"));
		Lines.Add(TEXT(""));
		Lines.Add(FString::Printf(TEXT("- Status: %s"), Report.bPassed ? TEXT("PASS") : TEXT("FAIL")));
		Lines.Add(FString::Printf(TEXT("- Summary: %s"), *Report.Summary));
		Lines.Add(FString::Printf(TEXT("- Manifest Source: `%s`"), *Report.ManifestSource));
		Lines.Add(FString::Printf(TEXT("- Systems: %d"), Report.SystemCount));
		Lines.Add(FString::Printf(TEXT("- Actions: %d"), Report.ActionCount));
		Lines.Add(FString::Printf(TEXT("- Errors: %d"), Report.ErrorCount));
		Lines.Add(FString::Printf(TEXT("- Warnings: %d"), Report.WarningCount));
		Lines.Add(TEXT(""));

		Lines.Add(TEXT("## Forge Metadata"));
		Lines.Add(TEXT(""));
		Lines.Add(FString::Printf(TEXT("- forgeVersion: %d"), Report.ForgeVersion));
		Lines.Add(FString::Printf(TEXT("- inputHash: %s"), *Report.InputHash));
		Lines.Add(FString::Printf(TEXT("- systemsCount: %d"), Report.SystemCount));
		Lines.Add(FString::Printf(TEXT("- actionsCount: %d"), Report.ActionCount));
		Lines.Add(TEXT(""));

		if (Report.Errors.Num() > 0)
		{
			Lines.Add(TEXT("## Issues"));
			Lines.Add(TEXT(""));
			for (const FOmniForgeError& Issue : Report.Errors)
			{
				Lines.Add(
					FString::Printf(
						TEXT("- [%s] `%s` at `%s`: %s | Recommendation: %s"),
						*SeverityToString(Issue.Severity),
						*Issue.Code,
						*Issue.Location,
						*Issue.Message,
						*Issue.Recommendation
					)
				);
			}
			Lines.Add(TEXT(""));
		}

		Lines.Add(TEXT("## Outputs"));
		Lines.Add(TEXT(""));
		Lines.Add(FString::Printf(TEXT("- ResolvedManifest: `%s`"), DisplayResolvedManifestPath));
		Lines.Add(FString::Printf(TEXT("- Report: `%s`"), DisplayReportPath));
		Lines.Add(TEXT(""));

		return FString::Join(Lines, TEXT("\n"));
	}

	static bool SaveTextFile(const FString& FilePath, const FString& Content, FString& OutError)
	{
		OutError.Reset();
		if (FFileHelper::SaveStringToFile(Content, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			return true;
		}

		OutError = FString::Printf(TEXT("Failed to write file: %s"), *FilePath);
		return false;
	}
}

static bool PhaseNormalize(FForgeContext& Context)
{
	Context.EffectiveInput = Context.Input;
	Context.EffectiveInput.GenerationRoot = OmniForge::NormalizeRootPath(Context.Input.GenerationRoot);

	Context.Manifest = nullptr;
	OmniForge::TryResolveManifest(Context.EffectiveInput, Context.Manifest, Context.Report);

	OmniForge::NormalizeManifest(Context.Manifest, Context.EffectiveInput, Context.Normalized);
	if (!Context.Report.HasErrors())
	{
		if (!OmniForge::ComputeNormalizedInputHash(Context.Normalized, Context.Report.InputHash))
		{
			Context.Report.AddError(
				TEXT("OMNI_FORGE_E099_INTERNAL"),
				TEXT("Failed to compute normalized manifest SHA256."),
				TEXT("Normalize"),
				TEXT("Inspect normalized manifest serialization and hashing.")
			);
		}
	}

	Context.InputHash = Context.Report.InputHash;
	return !Context.Report.HasErrors();
}

static bool PhaseValidate(FForgeContext& Context)
{
	if (!Context.Report.HasErrors())
	{
		OmniForge::ValidateAndResolve(
			Context.Normalized,
			Context.EffectiveInput.bRequireContentAssets,
			Context.InitializationOrder,
			Context.Profiles,
			Context.Actions,
			Context.Report
		);
	}

	return !Context.Report.HasErrors();
}

static bool PhaseResolve(FForgeContext& Context)
{
	if (!Context.Report.HasErrors())
	{
		OmniForge::BuildResolved(
			Context.Normalized,
			Context.InitializationOrder,
			Context.Profiles,
			Context.Actions,
			Context.Report.InputHash,
			Context.Resolved
		);
		Context.Report.SystemCount = Context.Resolved.SystemsCount;
		Context.Report.ActionCount = Context.Resolved.ActionsCount;
	}
	else
	{
		Context.Report.SystemCount = Context.Normalized.Systems.Num();
		Context.Report.ActionCount = Context.Actions.Num();
	}

	return !Context.Report.HasErrors();
}

static bool PhaseGenerate(FForgeContext& Context)
{
	Context.SavedOmniDir = FPaths::Combine(FPaths::ProjectSavedDir(), OmniForge::SavedFolder);
	IFileManager::Get().MakeDirectory(*Context.SavedOmniDir, true);
	Context.Report.OutputReportPath = FPaths::Combine(Context.SavedOmniDir, OmniForge::ReportFile);
	Context.Report.OutputResolvedManifestPath = FPaths::Combine(Context.SavedOmniDir, OmniForge::ResolvedManifestFile);
	Context.ResolvedManifestOutputFile = Context.Report.OutputResolvedManifestPath;

	if (!Context.Report.HasErrors())
	{
		FString JsonOutput;
		{
			if (!OmniForge::BuildResolvedJsonString(Context.Resolved, JsonOutput))
			{
				Context.Report.AddError(
					TEXT("OMNI_FORGE_E099_INTERNAL"),
					TEXT("Failed to serialize ResolvedManifest JSON."),
					TEXT("Generate"),
					TEXT("Inspect resolved manifest structure for unsupported fields.")
				);
			}
		}

		if (!Context.Report.HasErrors())
		{
			FString WriteError;
			if (!OmniForge::SaveTextFile(Context.Report.OutputResolvedManifestPath, JsonOutput, WriteError))
			{
				Context.Report.AddError(
					TEXT("OMNI_FORGE_E099_INTERNAL"),
					WriteError,
					TEXT("Generate"),
					TEXT("Ensure Saved/Omni is writable.")
				);
			}
		}
	}

	return !Context.Report.HasErrors();
}

static void PhaseReport(FForgeContext& Context)
{
	Context.Report.bPassed = !Context.Report.HasErrors();
	if (!Context.Report.bPassed)
	{
		IFileManager::Get().Delete(*Context.ResolvedManifestOutputFile, false, true, true);
		Context.Report.OutputResolvedManifestPath = TEXT("(not generated)");
	}

	Context.Report.Summary = Context.Report.bPassed
		? FString::Printf(
			TEXT("PASS: Normalize -> Validate -> Resolve -> Generate -> Report completed. Systems=%d Actions=%d."),
			Context.Report.SystemCount,
			Context.Report.ActionCount
		)
		: FString::Printf(
			TEXT("FAIL: pipeline aborted with %d error(s) and %d warning(s)."),
			Context.Report.ErrorCount,
			Context.Report.WarningCount
		);

	{
		const FString Markdown = OmniForge::BuildReportMarkdown(Context.Report);
		FString WriteError;
		if (!OmniForge::SaveTextFile(Context.Report.OutputReportPath, Markdown, WriteError))
		{
			UE_LOG(LogOmniForge, Error, TEXT("%s"), *WriteError);
		}
	}

	if (Context.Report.bPassed)
	{
		UE_LOG(
			LogOmniForge,
			Log,
			TEXT("Forge PASS. Systems=%d Actions=%d Resolved=%s Report=%s"),
			Context.Report.SystemCount,
			Context.Report.ActionCount,
			*Context.Report.OutputResolvedManifestPath,
			*Context.Report.OutputReportPath
		);
	}
	else
	{
		UE_LOG(
			LogOmniForge,
			Error,
			TEXT("Forge FAIL. Errors=%d Warnings=%d Report=%s"),
			Context.Report.ErrorCount,
			Context.Report.WarningCount,
			*Context.Report.OutputReportPath
		);

		for (const FOmniForgeError& Issue : Context.Report.Errors)
		{
			if (Issue.Severity == EOmniForgeErrorSeverity::Error)
			{
				UE_LOG(
					LogOmniForge,
					Error,
					TEXT("[%s] %s | Location=%s | Recommendation=%s"),
					*Issue.Code,
					*Issue.Message,
					*Issue.Location,
					*Issue.Recommendation
				);
			}
			else
			{
				UE_LOG(
					LogOmniForge,
					Warning,
					TEXT("[%s] %s | Location=%s | Recommendation=%s"),
					*Issue.Code,
					*Issue.Message,
					*Issue.Location,
					*Issue.Recommendation
				);
			}
		}
	}
}

static FOmniForgeReport RunInternal(const FOmniForgeInput& Input, FOmniForgeResolved* OutResolved)
{
	FForgeContext Context;
	Context.Input = Input;
	Context.Report.ForgeVersion = OmniForge::ForgeVersion;

	PhaseNormalize(Context);
	PhaseValidate(Context);
	PhaseResolve(Context);
	PhaseGenerate(Context);
	PhaseReport(Context);

	if (OutResolved)
	{
		if (Context.Report.bPassed)
		{
			*OutResolved = MoveTemp(Context.Resolved);
		}
		else
		{
			*OutResolved = FOmniForgeResolved();
		}
	}

	return Context.Report;
}

FOmniForgeResult UOmniForgeRunner::Run(const FOmniForgeInput& Input)
{
	FOmniForgeResult Result;
	Result.Report = RunInternal(Input, &Result.Resolved);
	Result.bSuccess = Result.Report.bPassed;
	Result.bHasResolvedManifest = Result.Report.bPassed;
	if (!Result.bHasResolvedManifest)
	{
		Result.Resolved = FOmniForgeResolved();
	}

	return Result;
}
