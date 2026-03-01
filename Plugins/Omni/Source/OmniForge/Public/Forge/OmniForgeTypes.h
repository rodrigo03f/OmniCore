#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

struct FOmniForgeInput
{
	FString GenerationRoot = TEXT("/Game/Data");
	FSoftObjectPath ManifestAssetPath;
	FSoftClassPath ManifestClassPath;
	bool bRequireContentAssets = true;
};

enum class EOmniForgeErrorSeverity : uint8
{
	Warning,
	Error
};

struct FOmniForgeError
{
	EOmniForgeErrorSeverity Severity = EOmniForgeErrorSeverity::Error;
	FString Code;
	FString Message;
	FString Location;
	FString Recommendation;
};

struct OMNIFORGE_API FOmniForgeNormalizedSystem
{
	FName SystemId = NAME_None;
	FString SystemClassPath;
	TArray<FName> Dependencies;

	void SetSetting(FName Key, const FString& Value);
	bool TryGetSetting(FName Key, FString& OutValue) const;
	bool TryGetSettingPtr(FName Key, const FString*& OutPtr) const;
	void GetSettingKeys(TArray<FName>& OutKeys) const;

private:
	TMap<FName, FString> Settings;
};

struct FOmniForgeNormalized
{
	FString GenerationRoot;
	FString Namespace;
	int32 BuildVersion = 0;
	TArray<FOmniForgeNormalizedSystem> Systems;
};

struct FOmniForgeResolvedSystem
{
	FName SystemId = NAME_None;
	FString SystemClassPath;
	TArray<FName> Dependencies;
};

struct FOmniForgeResolvedProfile
{
	FName SystemId = NAME_None;
	FName SettingKey = NAME_None;
	FString ProfileAssetPath;
	FString ProfileClassPath;
	FString LibraryAssetPath;
	FString LibraryClassPath;
};

struct FOmniForgeResolvedAction
{
	FName ActionId = NAME_None;
	bool bEnabled = true;
	FString Policy;
	TArray<FString> BlockedBy;
	TArray<FName> Cancels;
	TArray<FString> AppliesLocks;
};

struct FOmniForgeResolved
{
	int32 ForgeVersion = 1;
	FString InputHash;
	int32 SystemsCount = 0;
	int32 ActionsCount = 0;
	FString GenerationRoot;
	FString Namespace;
	int32 BuildVersion = 0;
	TArray<FOmniForgeResolvedSystem> Systems;
	TArray<FName> InitializationOrder;
	TArray<FOmniForgeResolvedProfile> Profiles;
	TArray<FOmniForgeResolvedAction> ActionDefinitions;
};

struct FOmniForgeReport
{
	bool bPassed = false;
	int32 ForgeVersion = 1;
	FString InputHash;
	FString Summary;
	FString ManifestSource;
	FString OutputResolvedManifestPath;
	FString OutputGeneratedPath;
	FString OutputReportPath;
	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 SystemCount = 0;
	int32 ActionCount = 0;
	TArray<FOmniForgeError> Errors;

	void AddIssue(
		const EOmniForgeErrorSeverity Severity,
		const FString& Code,
		const FString& Message,
		const FString& Location,
		const FString& Recommendation
	)
	{
		FOmniForgeError& ErrorEntry = Errors.AddDefaulted_GetRef();
		ErrorEntry.Severity = Severity;
		ErrorEntry.Code = Code;
		ErrorEntry.Message = Message;
		ErrorEntry.Location = Location;
		ErrorEntry.Recommendation = Recommendation;

		if (Severity == EOmniForgeErrorSeverity::Error)
		{
			++ErrorCount;
		}
		else
		{
			++WarningCount;
		}
	}

	void AddError(const FString& Code, const FString& Message, const FString& Location, const FString& Recommendation)
	{
		AddIssue(EOmniForgeErrorSeverity::Error, Code, Message, Location, Recommendation);
	}

	void AddWarning(const FString& Code, const FString& Message, const FString& Location, const FString& Recommendation)
	{
		AddIssue(EOmniForgeErrorSeverity::Warning, Code, Message, Location, Recommendation);
	}

	bool HasErrors() const
	{
		return ErrorCount > 0;
	}
};

struct FOmniForgeResult
{
	bool bSuccess = false;
	bool bHasResolvedManifest = false;
	FOmniForgeResolved Resolved;
	FOmniForgeReport Report;
};
