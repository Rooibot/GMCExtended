﻿#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGMCExtendedAnimationModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

DECLARE_LOG_CATEGORY_EXTERN(LogGMCExAnimation, Log, All)