# OmniForge / OmniForgeModule

## Arquivos agrupados
- Plugins/Omni/Source/OmniForge/Private/OmniForgeModule.cpp
- Plugins/Omni/Source/OmniForge/Public/OmniForgeModule.h

## Header: Plugins/Omni/Source/OmniForge/Public/OmniForgeModule.h
```cpp
#pragma once

#include "Modules/ModuleManager.h"

class FOmniForgeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

```

## Source: Plugins/Omni/Source/OmniForge/Private/OmniForgeModule.cpp
```cpp
#include "OmniForgeModule.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FOmniForgeModule, OmniForge)

void FOmniForgeModule::StartupModule()
{
}

void FOmniForgeModule::ShutdownModule()
{
}

```

