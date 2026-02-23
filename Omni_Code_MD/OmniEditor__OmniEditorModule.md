# OmniEditor / OmniEditorModule

## Arquivos agrupados
- Plugins/Omni/Source/OmniEditor/Private/OmniEditorModule.cpp
- Plugins/Omni/Source/OmniEditor/Public/OmniEditorModule.h

## Header: Plugins/Omni/Source/OmniEditor/Public/OmniEditorModule.h
```cpp
#pragma once

#include "Modules/ModuleManager.h"

class FOmniEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

```

## Source: Plugins/Omni/Source/OmniEditor/Private/OmniEditorModule.cpp
```cpp
#include "OmniEditorModule.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FOmniEditorModule, OmniEditor)

void FOmniEditorModule::StartupModule()
{
}

void FOmniEditorModule::ShutdownModule()
{
}

```

