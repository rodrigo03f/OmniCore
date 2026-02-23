# OmniCore / OmniCoreModule

## Arquivos agrupados
- Plugins/Omni/Source/OmniCore/Private/OmniCoreModule.cpp
- Plugins/Omni/Source/OmniCore/Public/OmniCoreModule.h

## Header: Plugins/Omni/Source/OmniCore/Public/OmniCoreModule.h
```cpp
#pragma once

#include "Modules/ModuleManager.h"

class FOmniCoreModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

```

## Source: Plugins/Omni/Source/OmniCore/Private/OmniCoreModule.cpp
```cpp
#include "OmniCoreModule.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FOmniCoreModule, OmniCore)

void FOmniCoreModule::StartupModule()
{
}

void FOmniCoreModule::ShutdownModule()
{
}

```

