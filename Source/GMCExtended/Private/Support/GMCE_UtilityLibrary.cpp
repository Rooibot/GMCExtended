#include "Support/GMCE_UtilityLibrary.h"

#include "GMCPlayerController.h"
#include "Kismet/KismetMathLibrary.h"

float UGMCE_UtilityLibrary::GetAngleDifferenceXY(const FVector& A, const FVector& B)
{
	const FVector XY=FVector(1.f, 1.f, 0.f);
	const FVector AXY = A * XY;
	const FVector BXY = B * XY;
	
	float Angle = GetAngleDifference(AXY, BXY);
	const FVector Cross = FVector::CrossProduct(AXY, BXY);
	Angle *= Cross.Z > 0.f ? 1.f : -1.f;
	return UKismetMathLibrary::NormalizeAxis(Angle);	
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

// 
FTrajectorySample UGMCE_UtilityLibrary::ConvertMovementSampleToTrajectorySample(const FGMCE_MovementSample& Sample)
{
	// Just use the FGMCE_MovementSample's own conversion operator.
	return static_cast<FTrajectorySample>(Sample);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
FTrajectorySampleRange UGMCE_UtilityLibrary::ConvertMovementSampleCollectionToTrajectorySampleRange(
	const FGMCE_MovementSampleCollection& MovementSampleCollection)
{
	// Just use the FGMCE_MovementSampleRange's own conversion operator.
	return static_cast<FTrajectorySampleRange>(MovementSampleCollection);
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

FPoseSearchQueryTrajectorySample UGMCE_UtilityLibrary::ConvertMovementSampleCollectionToPoseSearchQueryTrajectorySample(
	const FGMCE_MovementSample& MovementSample)
{
	// Just use the FPoseSearchQueryTrajectorySample's own conversion operator.
	return static_cast<FPoseSearchQueryTrajectorySample>(MovementSample);
}

FPoseSearchQueryTrajectory UGMCE_UtilityLibrary::ConvertMovementSampleCollectionToPoseSearchQueryTrajectory(
	const FGMCE_MovementSampleCollection& MovementSampleCollection)
{
	// Just use the FPoseSearchQueryTrajectory's own conversion operator.
	return static_cast<FPoseSearchQueryTrajectory>(MovementSampleCollection);
}

float UGMCE_UtilityLibrary::GetSynchronizedWorldTime(UObject* WorldContextObject)
{
	UWorld* World;

	if (WorldContextObject)
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	}
	else
	{
		// If we aren't given a world context object, make a best effort.
		World = GEngine->GetCurrentPlayWorld();
	}

	// If we have no world, or our world isn't a game world, we don't have any world time.
	if (!World || !World->IsGameWorld()) return 0.f;

	// If we aren't a client, we ARE the authoritative time.
	if (World->GetNetMode() != NM_Client) return World->GetRealTimeSeconds();

	// Find a local player controller.
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) return 0.f;

	const AGMC_PlayerController *Controller = Cast<AGMC_PlayerController>(GameInstance->GetFirstLocalPlayerController(World));
	if (!Controller) return 0.f;

	// Get the GMC-provided synchronized server world time.
	return Controller->CL_GetSyncedWorldTimeSeconds();
}
