# OmniGAS / OmniGASModule

## Arquivos agrupados
- Plugins/Omni/Source/OmniGAS/Private/OmniGASModule.cpp
- Plugins/Omni/Source/OmniGAS/Public/OmniGASModule.h

## Header: Plugins/Omni/Source/OmniGAS/Public/OmniGASModule.h
```cpp
#pragma once

#include "Modules/ModuleManager.h"

class FOmniGASModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

```

## Source: Plugins/Omni/Source/OmniGAS/Private/OmniGASModule.cpp
```cpp
#include "OmniGASModule.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FOmniGASModule, OmniGAS)

void FOmniGASModule::StartupModule()
{
}

void FOmniGASModule::ShutdownModule()
{
}

```

