# OmniRuntime / OmniOfficialManifest

## Arquivos agrupados
- Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp
- Plugins/Omni/Source/OmniRuntime/Public/Manifest/OmniOfficialManifest.h

## Header: Plugins/Omni/Source/OmniRuntime/Public/Manifest/OmniOfficialManifest.h
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Manifest/OmniManifest.h"
#include "OmniOfficialManifest.generated.h"

UCLASS(BlueprintType)
class OMNIRUNTIME_API UOmniOfficialManifest : public UOmniManifest
{
	GENERATED_BODY()

public:
	UOmniOfficialManifest();
};

```

## Source: Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp
```cpp
#include "Manifest/OmniOfficialManifest.h"

#include "Systems/ActionGate/OmniActionGateSystem.h"
#include "Systems/Movement/OmniMovementSystem.h"
#include "Systems/Status/OmniStatusSystem.h"

UOmniOfficialManifest::UOmniOfficialManifest()
{
	if (Systems.Num() > 0)
	{
		return;
	}

	Namespace = TEXT("Omni.Official");
	BuildVersion = 1;

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Status");
		Entry.SystemClass = UOmniStatusSystem::StaticClass();
		Entry.bEnabled = true;
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("ActionGate");
		Entry.SystemClass = UOmniActionGateSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("Status") };
	}

	{
		FOmniSystemManifestEntry& Entry = Systems.AddDefaulted_GetRef();
		Entry.SystemId = TEXT("Movement");
		Entry.SystemClass = UOmniMovementSystem::StaticClass();
		Entry.bEnabled = true;
		Entry.Dependencies = { TEXT("ActionGate"), TEXT("Status") };
	}
}

```

