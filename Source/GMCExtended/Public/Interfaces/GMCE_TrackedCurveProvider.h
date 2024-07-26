// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GMCE_TrackedCurveProvider.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGMCE_TrackedCurveProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GMCEXTENDED_API IGMCE_TrackedCurveProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual float GetTrackedCurve(const FName CurveName) const;
	virtual bool GetTrackedCurve(const FName CurveName, float& OutValue) const;
	
};
