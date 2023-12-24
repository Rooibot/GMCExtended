// Fill out your copyright notice in the Description page of Project Settings.

// -- the below comment for ReSharper is just to make it stop whining about "AXY" and "BXY" and so on not
//    being "appropriate" UE names, and constantly trying to change them to Axy and Bxy and so on.
//
// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "Support/GMCE_UtilityLibrary.h"

float UGMCE_UtilityLibrary::GetAngleDifferenceXY(const FVector& A, const FVector& B)
{
	const FVector XY=FVector(1.f, 1.f, 0.f);
	const FVector AXY = A * XY;
	const FVector BXY = B * XY;
	
	float Angle = GetAngleDifference(AXY, BXY);
	const FVector Cross = FVector::CrossProduct(AXY, BXY);
	Angle *= Cross.Z > 0.f ? 1.f : -1.f;
	return Angle;	
}

float UGMCE_UtilityLibrary::GetAngleDifferenceZ(const FVector& A, const FVector& B)
{
	const FVector Z=FVector(0.f, 0.f, 1.f);
	const FVector AZ = A * Z;
	const FVector BZ = B * Z;

	float Angle = GetAngleDifference(AZ, BZ);
	const FVector Cross = FVector::CrossProduct(AZ, BZ);
	Angle *= Cross.Y > 0.f ? 1.f : -1.f;

	return Angle;
}

float UGMCE_UtilityLibrary::GetAngleDifference(const FVector& A, const FVector& B)
{
	const FVector ANorm = A.GetSafeNormal();
	const FVector BNorm = B.GetSafeNormal();

	// Why waste time on math if they're identical?
	if (ANorm == BNorm) return 0.f;

	const float DotProduct = FVector::DotProduct(ANorm, BNorm);
	return (180.0)/UE_DOUBLE_PI * FMath::Acos(DotProduct);	
}

FTrajectorySample UGMCE_UtilityLibrary::ConvertMovementSampleToTrajectorySample(const FGMCE_MovementSample& Sample)
{
	// Just use the FGMCE_MovementSample's own conversion operator.
	return static_cast<FTrajectorySample>(Sample);
}

FTrajectorySampleRange UGMCE_UtilityLibrary::ConvertMovementSampleCollectionToTrajectorySampleRange(
	const FGMCE_MovementSampleCollection& MovementSampleCollection)
{
	// Just use the FGMCE_MovementSampleRange's own conversion operator.
	return static_cast<FTrajectorySampleRange>(MovementSampleCollection);
}
