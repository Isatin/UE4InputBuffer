// Copyright 2017 Isaac Hsu. MIT License

#include "InputBufferPrivatePCH.h"

#define LOCTEXT_NAMESPACE "InputBuffer"

DEFINE_LOG_CATEGORY(InputBufferLog)

void FInputBufferModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FInputBufferModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInputBufferModule, InputBuffer)