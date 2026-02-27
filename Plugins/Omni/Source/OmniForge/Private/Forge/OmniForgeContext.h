#pragma once

#include "CoreMinimal.h"
#include "Forge/OmniForgeTypes.h"

class UOmniManifest;

struct FForgeContext
{
	FOmniForgeInput Input;
	FOmniForgeInput EffectiveInput;
	UOmniManifest* Manifest = nullptr;

	FOmniForgeNormalized Normalized;
	FString InputHash;
	TArray<FName> InitializationOrder;
	TArray<FOmniForgeResolvedProfile> Profiles;
	TArray<FOmniForgeResolvedAction> Actions;
	FOmniForgeResolved Resolved;

	FOmniForgeReport Report;
	FString SavedOmniDir;
	FString ResolvedManifestOutputFile;
};
