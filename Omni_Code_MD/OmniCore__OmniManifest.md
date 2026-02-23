# OmniCore / OmniManifest

## Arquivos agrupados
- Plugins/Omni/Source/OmniCore/Private/Manifest/OmniManifest.cpp
- Plugins/Omni/Source/OmniCore/Public/Manifest/OmniManifest.h

## Header: Plugins/Omni/Source/OmniCore/Public/Manifest/OmniManifest.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"
#include "OmniManifest.generated.h"

class UOmniRuntimeSystem;

USTRUCT(BlueprintType)
struct OMNICORE_API FOmniSystemManifestEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Manifest")
	FName SystemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Manifest")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Manifest")
	TSoftClassPtr<UOmniRuntimeSystem> SystemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Manifest")
	TArray<FName> Dependencies;
};

UCLASS(BlueprintType)
class OMNICORE_API UOmniManifest : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Manifest")
	FName Namespace = TEXT("Omni");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Manifest")
	int32 BuildVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Omni|Manifest")
	TArray<FOmniSystemManifestEntry> Systems;

	const FOmniSystemManifestEntry* FindEntryById(FName SystemId) const;
};


```

## Source: Plugins/Omni/Source/OmniCore/Private/Manifest/OmniManifest.cpp
```cpp
#include "Manifest/OmniManifest.h"

const FOmniSystemManifestEntry* UOmniManifest::FindEntryById(const FName SystemId) const
{
	if (SystemId == NAME_None)
	{
		return nullptr;
	}

	return Systems.FindByPredicate(
		[SystemId](const FOmniSystemManifestEntry& Entry)
		{
			return Entry.SystemId == SystemId;
		}
	);
}


```

