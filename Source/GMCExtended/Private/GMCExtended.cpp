// Copyright Epic Games, Inc. All Rights Reserved.

#include "GMCExtended.h"
#include "GMCExtendedLog.h"

#define LOCTEXT_NAMESPACE "FGMCExtendedModule"

void FGMCExtendedModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FGMCExtendedModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGMCExtendedModule, GMCExtended)
DEFINE_LOG_CATEGORY(LogGMCExtended)