// LICENSE_CODE ZON

#pragma once

#include "Modules/ModuleManager.h"

class FBrightSDKModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
