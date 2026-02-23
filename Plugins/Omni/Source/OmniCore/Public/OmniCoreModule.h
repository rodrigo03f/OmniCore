#pragma once

#include "Modules/ModuleManager.h"

class FOmniCoreModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
