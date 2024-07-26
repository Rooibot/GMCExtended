// Copyright 2024 Rooibot Games, LLC


#include "GMCE_TrackedCurveProvider.h"


// Add default functionality here for any IGMCE_TrackedCurveProvider functions that are not pure virtual.
float IGMCE_TrackedCurveProvider::GetTrackedCurve(const FName CurveName) const
{
	return 0.f;
}

bool IGMCE_TrackedCurveProvider::GetTrackedCurve(const FName CurveName, float& OutValue) const
{
	OutValue = 0.f;
	return false;
}
