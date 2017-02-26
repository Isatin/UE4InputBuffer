// Copyright 2017 Isaac Hsu. MIT License

#pragma once

#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(InputBufferLog, Log, All);

class FInputBufferModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};