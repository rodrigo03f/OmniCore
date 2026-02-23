#pragma once

#include "Modules/ModuleManager.h"

class FOmniGASModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
