#include "Modifiers/GMCE_RootMotionModifier_SkewWarp.h"

#include "GMCExtendedAnimationLog.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_MotionWarpingUtilities.h"
#include "GMCE_MotionWarpSubject.h"

UGMCE_RootMotionModifier_SkewWarp::UGMCE_RootMotionModifier_SkewWarp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FTransform UGMCE_RootMotionModifier_SkewWarp::ProcessRootMotion(const FTransform& InRootMotion, const FGMCE_MotionWarpContext& WarpContext)
{
	FTransform FinalRootMotion = InRootMotion;

	const FTransform RootMotionTotal = UGMCE_MotionWarpingUtilities::ExtractRootMotionFromAnimation(AnimationSequence.Get(), PreviousPosition, EndTime);
	const FTransform RootMotionDelta = UGMCE_MotionWarpingUtilities::ExtractRootMotionFromAnimation(AnimationSequence.Get(), PreviousPosition, FMath::Min(CurrentPosition, EndTime));

	FTransform ExtraRootMotion = FTransform::Identity;
	if (CurrentPosition > EndTime)
	{
		ExtraRootMotion = UGMCE_MotionWarpingUtilities::ExtractRootMotionFromAnimation(AnimationSequence.Get(), EndTime, CurrentPosition);
	}

	if (bWarpTranslation)
	{
		const float CapsuleHalfHeight = WarpContext.CapsuleHalfHeight;
		const FQuat CurrentRotation = WarpContext.OwnerTransform.GetRotation();
		const FVector CurrentLocation = WarpContext.OwnerTransform.GetLocation() - CurrentRotation.GetUpVector() * CapsuleHalfHeight;

		const FVector DeltaTranslation = RootMotionDelta.GetLocation();
		const FVector TotalTranslation = RootMotionTotal.GetLocation();

		FVector TargetLocation = GetTargetLocation();
		if (bIgnoreZAxis)
		{
			TargetLocation.Z = CurrentLocation.Z;
		}

		// if there is translation in the animation, warp it
		if (!TotalTranslation.IsNearlyZero())
		{
			if (!DeltaTranslation.IsNearlyZero())
			{
				const FTransform MeshTransform = FTransform(WarpContext.MeshRelativeTransform.GetRotation(), WarpContext.MeshRelativeTransform.GetTranslation());
				const FTransform FinalMeshTransform = WarpContext.MeshRelativeTransform * WarpContext.OwnerTransform;
				TargetLocation = FinalMeshTransform.InverseTransformPositionNoScale(TargetLocation);

				const FVector WarpedTranslation = WarpTranslation(FTransform::Identity, DeltaTranslation, TotalTranslation, TargetLocation) + ExtraRootMotion.GetLocation();
				FinalRootMotion.SetTranslation(WarpedTranslation);
			}
		}
		// if there is no translation in the animation, add it
		else
		{
			const FVector DeltaToTarget = TargetLocation - CurrentLocation;
			if (DeltaToTarget.IsNearlyZero())
			{
				FinalRootMotion.SetTranslation(FVector::ZeroVector);
			}
			else
			{
				float Alpha = FMath::Clamp((CurrentPosition - ActualStartTime) / (EndTime - ActualStartTime), 0.f, 1.f);
				Alpha = FAlphaBlend::AlphaToBlendOption(Alpha, AddTranslationEasingFunc, AddTranslationEasingCurve);

				const FVector NextLocation = FMath::Lerp<FVector, float>(StartTransform.GetLocation(), TargetLocation, Alpha);
				FVector FinalDeltaTranslation = (NextLocation - CurrentLocation);
				FinalDeltaTranslation = (CurrentRotation.Inverse() * DeltaToTarget.ToOrientationQuat()).GetForwardVector() * FinalDeltaTranslation.Size();
				FinalDeltaTranslation = WarpContext.MeshRelativeTransform.GetRotation().UnrotateVector(FinalDeltaTranslation);

				FinalRootMotion.SetTranslation(FinalDeltaTranslation + ExtraRootMotion.GetLocation());
			}
		}
	}

	if(bWarpRotation)
	{
		const FQuat WarpedRotation = ExtraRootMotion.GetRotation() * WarpRotation(WarpContext, RootMotionDelta, RootMotionTotal, WarpContext.DeltaSeconds);
		FinalRootMotion.SetRotation(WarpedRotation);
	}

	// Debug
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const int32 DebugLevel = FGMCE_MotionWarpCvars::CVarMotionWarpingDebug.GetValueOnGameThread();
	if (DebugLevel == 1 || DebugLevel == 3)
	{
		PrintLog(TEXT("GMCE_SkewWarp"), InRootMotion, FinalRootMotion);
	}

	if (DebugLevel >= 2)
	{
		const float DrawDebugDuration = FGMCE_MotionWarpCvars::CVarMotionWarpingDrawDebugDuration.GetValueOnGameThread();
		DrawDebugCoordinateSystem(GetPawnOwner()->GetWorld(), GetTargetLocation(), GetTargetRotator(WarpContext), 50.f, false, DrawDebugDuration, 0, 1.f);
	}
#endif
	

	return FinalRootMotion;
}

FVector UGMCE_RootMotionModifier_SkewWarp::WarpTranslation(const FTransform& CurrentTransform,
	const FVector& DeltaTranslation, const FVector& TotalTranslation, const FVector& TargetLocation)
{
	if (!DeltaTranslation.IsNearlyZero())
	{
		const FQuat CurrentRotation = CurrentTransform.GetRotation();
		const FVector CurrentLocation = CurrentTransform.GetLocation();
		const FVector FutureLocation = CurrentLocation + TotalTranslation;
		const FVector CurrentToWorldOffset = TargetLocation - CurrentLocation;
		const FVector CurrentToRootOffset = FutureLocation - CurrentLocation;

		// Create a matrix we can use to put everything into a space looking straight at RootMotionSyncPosition. "forward" should be the axis along which we want to scale. 
		FVector ToRootNormalized = CurrentToRootOffset.GetSafeNormal();

		float BestMatchDot = FMath::Abs(FVector::DotProduct(ToRootNormalized, CurrentRotation.GetAxisX()));
		FMatrix ToRootSyncSpace = FRotationMatrix::MakeFromXZ(ToRootNormalized, CurrentRotation.GetAxisZ());

		float ZDot = FMath::Abs(FVector::DotProduct(ToRootNormalized, CurrentRotation.GetAxisZ()));
		if (ZDot > BestMatchDot)
		{
			ToRootSyncSpace = FRotationMatrix::MakeFromXZ(ToRootNormalized, CurrentRotation.GetAxisX());
			BestMatchDot = ZDot;
		}

		float YDot = FMath::Abs(FVector::DotProduct(ToRootNormalized, CurrentRotation.GetAxisY()));
		if (YDot > BestMatchDot)
		{
			ToRootSyncSpace = FRotationMatrix::MakeFromXZ(ToRootNormalized, CurrentRotation.GetAxisZ());
		}

		// Put everything into RootSyncSpace.
		const FVector RootMotionInSyncSpace = ToRootSyncSpace.InverseTransformVector(DeltaTranslation);
		const FVector CurrentToWorldSync = ToRootSyncSpace.InverseTransformVector(CurrentToWorldOffset);
		const FVector CurrentToRootMotionSync = ToRootSyncSpace.InverseTransformVector(CurrentToRootOffset);

		FVector CurrentToWorldSyncNorm = CurrentToWorldSync;
		CurrentToWorldSyncNorm.Normalize();

		FVector CurrentToRootMotionSyncNorm = CurrentToRootMotionSync;
		CurrentToRootMotionSyncNorm.Normalize();

		// Calculate skew Yaw Angle. 
		FVector FlatToWorld = FVector(CurrentToWorldSyncNorm.X, CurrentToWorldSyncNorm.Y, 0.0f);
		FlatToWorld.Normalize();
		FVector FlatToRoot = FVector(CurrentToRootMotionSyncNorm.X, CurrentToRootMotionSyncNorm.Y, 0.0f);
		FlatToRoot.Normalize();
		float AngleAboutZ = FMath::Acos(FVector::DotProduct(FlatToWorld, FlatToRoot));
		float AngleAboutZNorm = FMath::DegreesToRadians(FRotator::NormalizeAxis(FMath::RadiansToDegrees(AngleAboutZ)));
		if (FlatToWorld.Y < 0.0f)
		{
			AngleAboutZNorm *= -1.0f;
		}

		// Calculate Skew Pitch Angle. 
		FVector ToWorldNoY = FVector(CurrentToWorldSyncNorm.X, 0.0f, CurrentToWorldSyncNorm.Z);
		ToWorldNoY.Normalize();
		FVector ToRootNoY = FVector(CurrentToRootMotionSyncNorm.X, 0.0f, CurrentToRootMotionSyncNorm.Z);
		ToRootNoY.Normalize();
		const float AngleAboutY = FMath::Acos(FVector::DotProduct(ToWorldNoY, ToRootNoY));
		float AngleAboutYNorm = FMath::DegreesToRadians(FRotator::NormalizeAxis(FMath::RadiansToDegrees(AngleAboutY)));
		if (ToWorldNoY.Z < 0.0f)
		{
			AngleAboutYNorm *= -1.0f;
		}

		FVector SkewedRootMotion = FVector::ZeroVector;
		float ProjectedScale = FVector::DotProduct(CurrentToWorldSync, CurrentToRootMotionSyncNorm) / CurrentToRootMotionSync.Size();
		if (ProjectedScale != 0.0f)
		{
			FMatrix ScaleMatrix;
			ScaleMatrix.SetIdentity();
			ScaleMatrix.SetAxis(0, FVector(ProjectedScale, 0.0f, 0.0f));
			ScaleMatrix.SetAxis(1, FVector(0.0f, 1.0f, 0.0f));
			ScaleMatrix.SetAxis(2, FVector(0.0f, 0.0f, 1.0f));

			FMatrix ShearXAlongYMatrix;
			ShearXAlongYMatrix.SetIdentity();
			ShearXAlongYMatrix.SetAxis(0, FVector(1.0f, FMath::Tan(AngleAboutZNorm), 0.0f));
			ShearXAlongYMatrix.SetAxis(1, FVector(0.0f, 1.0f, 0.0f));
			ShearXAlongYMatrix.SetAxis(2, FVector(0.0f, 0.0f, 1.0f));

			FMatrix ShearXAlongZMatrix;
			ShearXAlongZMatrix.SetIdentity();
			ShearXAlongZMatrix.SetAxis(0, FVector(1.0f, 0.0f, FMath::Tan(AngleAboutYNorm)));
			ShearXAlongZMatrix.SetAxis(1, FVector(0.0f, 1.0f, 0.0f));
			ShearXAlongZMatrix.SetAxis(2, FVector(0.0f, 0.0f, 1.0f));

			FMatrix ScaledSkewMatrix = ScaleMatrix * ShearXAlongYMatrix * ShearXAlongZMatrix;

			// Skew and scale the Root motion. 
			SkewedRootMotion = ScaledSkewMatrix.TransformVector(RootMotionInSyncSpace);
		}
		else if (!CurrentToRootMotionSync.IsZero() && !CurrentToWorldSync.IsZero() && !RootMotionInSyncSpace.IsZero())
		{
			// Figure out ratio between remaining Root and remaining World. Then project scaled length of current Root onto World.
			const float Scale = CurrentToWorldSync.Size() / CurrentToRootMotionSync.Size();
			const float StepTowardTarget = RootMotionInSyncSpace.ProjectOnTo(RootMotionInSyncSpace).Size();
			SkewedRootMotion = CurrentToWorldSyncNorm * (Scale * StepTowardTarget);
		}

		// Put our result back in world space.  
		return ToRootSyncSpace.TransformVector(SkewedRootMotion);
	}

	return FVector::ZeroVector;	
}

UGMCE_RootMotionModifier_SkewWarp* UGMCE_RootMotionModifier_SkewWarp::AddRootMotionModifierSkewWarp(
	UGMCE_MotionWarpingComponent* InMotionWarpingComp, const UAnimSequenceBase* InAnimation, float InStartTime,
	float InEndTime, FName InWarpTargetName, EGMCE_MotionWarpProvider InWarpPointAnimProvider,
	FTransform InWarpPointAnimTransform, FName InWarpPointAnimBoneName, bool bInWarpTranslation, bool bInIgnoreZAxis,
	bool bInWarpRotation, EGMCE_MotionWarpRotationType InRotationType, EGMCE_MotionWarpRotationMethod InRotationMethod,
	float InWarpRotationTimeMultiplier, float InWarpMaxRotationRate)
{
	if (ensureAlways(InMotionWarpingComp))
	{
		UGMCE_RootMotionModifier_SkewWarp* NewModifier = NewObject<UGMCE_RootMotionModifier_SkewWarp>(InMotionWarpingComp);
		NewModifier->AnimationSequence = InAnimation;
		NewModifier->StartTime = InStartTime;
		NewModifier->EndTime = InEndTime;
		NewModifier->WarpTargetName = InWarpTargetName;
		NewModifier->WarpPointAnimProvider = InWarpPointAnimProvider;
		NewModifier->WarpPointAnimTransform = InWarpPointAnimTransform;
		NewModifier->WarpPointAnimBoneName = InWarpPointAnimBoneName;
		NewModifier->bWarpTranslation = bInWarpTranslation;
		NewModifier->bIgnoreZAxis = bInIgnoreZAxis;
		NewModifier->bWarpRotation = bInWarpRotation;
		NewModifier->RotationType = InRotationType;
		NewModifier->RotationMethod = InRotationMethod;
		NewModifier->WarpRotationTimeMultiplier = InWarpRotationTimeMultiplier;
		NewModifier->WarpMaxRotationRate = InWarpMaxRotationRate;

		InMotionWarpingComp->AddModifier(NewModifier);

		return NewModifier;
	}

	return nullptr;	
}
