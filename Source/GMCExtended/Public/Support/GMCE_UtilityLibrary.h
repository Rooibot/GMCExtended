// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GMCE_UtilityLibrary.generated.h"

/**
 * @brief A Blueprint Function Library providing useful/support functions for GMCExtended.
 */
UCLASS()
class GMCEXTENDED_API UGMCE_UtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/// Returns the angle, in degrees, by which vector B differs from vector A on the XY plane.
	/// In other words, the amount by which the yaw between them changes.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	static float GetAngleDifferenceXY(const FVector& A, const FVector& B);

	/// Returns the angle, in degrees, by which vector B differs from vector A on the Z plane.
	/// In other words, the ascent or descent.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	static float GetAngleDifferenceZ(const FVector& A, const FVector& B);

	/// Returns the angle, in degrees, by which vector B differs from vector A along a plane
	/// defined by their cross product.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	static float GetAngleDifference(const FVector& A, const FVector& B);
	

};
