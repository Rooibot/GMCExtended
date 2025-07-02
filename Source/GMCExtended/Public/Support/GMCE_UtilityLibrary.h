#pragma once

#include "CoreMinimal.h"
#include "GMCEMovementSample.h"
#include "Animation/MotionTrajectoryTypes.h"
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

	/// Converts a GMCEx Movement Sample into an Epic Trajectory Sample.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	static FTrajectorySample ConvertMovementSampleToTrajectorySample(const FGMCE_MovementSample& MovementSample);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS	
	/// Converts a GMCEx Movement Sample Collection into an Epic Trajectory Sample Range.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	static FTrajectorySampleRange ConvertMovementSampleCollectionToTrajectorySampleRange(const FGMCE_MovementSampleCollection& MovementSampleCollection);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	/// Converts a GMCEx Movement Sample into an Epic Pose Search Query Trajectory Sample.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	static FPoseSearchQueryTrajectorySample ConvertMovementSampleCollectionToPoseSearchQueryTrajectorySample(const FGMCE_MovementSample& MovementSample);
	
	/// Converts a GMCEx Movement Sample Collection into an Epic Pose Search Query Trajectory.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	static FPoseSearchQueryTrajectory ConvertMovementSampleCollectionToPoseSearchQueryTrajectory(const FGMCE_MovementSampleCollection& MovementSampleCollection);

	/// Get the server's RealTimeSeconds, as synchronized by GMC.
	UFUNCTION(BlueprintPure, Category="Time")
	static float GetSynchronizedWorldTime(UObject *WorldContextObject);

};
