// Copyright 2024 Rooibot Games, LLC


#include "GMCE_RootMotionModifier_Warp.h"
#include "GMCExtendedAnimationLog.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_MotionWarpingUtilities.h"

UGMCE_RootMotionModifier_Warp::UGMCE_RootMotionModifier_Warp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UGMCE_RootMotionModifier_Warp::Update(const FGMCE_MotionWarpContext& Context)
{
	Super::Update(Context);

	const UGMCE_MotionWarpingComponent* OwnerComp = GetOwnerComponent();
	if (OwnerComp && GetState() == EGMCE_RootMotionModifierState::Active)
	{
		const FGMCE_MotionWarpTarget* WarpTargetPtr = OwnerComp->FindWarpTarget(WarpTargetName);

		// Disable if there is no target for us
		if (WarpTargetPtr == nullptr)
		{
			UE_LOG(LogGMCExAnimation, Verbose, TEXT("MotionWarping: Marking RootMotionModifier as Disabled. Reason: Invalid Warp Target (%s). Char: %s Animation: %s [%f %f] [%f %f]"),
				*WarpTargetName.ToString(), *GetNameSafe(OwnerComp->GetOwner()), *GetNameSafe(AnimationSequence.Get()), StartTime, EndTime, PreviousPosition, CurrentPosition);

			SetState(EGMCE_RootMotionModifierState::Disabled);
			return;
		}

		// Get the warp point sent by the game
		FTransform WarpPointTransformGame = WarpTargetPtr->GetTargetTransform();
		FTransform Other = WarpTargetPtr->GetTargetTransformFromAnimation(Context.OwnerTransform, Context.MeshRelativeTransform, Context.AnimationInstance, GetAnimation(), CurrentPosition);

		// Initialize our target transform (where the root should end at the end of the window) with the warp point sent by the game
		FTransform TargetTransform = Other;

		// Check if a warp point is defined in the animation. If so, we need to extract it and offset the target transform 
		// the same amount the root bone is offset from the warp point in the animation
		if (WarpPointAnimProvider != EGMCE_MotionWarpProvider::None)
		{
			if (!CachedOffsetFromWarpPoint.IsSet())
			{
				if (AGMC_Pawn* PawnOwner = GetPawnOwner())
				{
					if (WarpPointAnimProvider == EGMCE_MotionWarpProvider::Static)
					{
						// CachedOffsetFromWarpPoint = UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(PawnOwner, GetAnimation(), EndTime, WarpPointAnimTransform);
						CachedOffsetFromWarpPoint = UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(Context.MeshRelativeTransform, GetAnimation(), EndTime, WarpPointAnimTransform);

					}
					else if (WarpPointAnimProvider == EGMCE_MotionWarpProvider::Bone)
					{
						// CachedOffsetFromWarpPoint = UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(PawnOwner, GetAnimation(), EndTime, WarpPointAnimBoneName);
						CachedOffsetFromWarpPoint = UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(Context.MeshRelativeTransform, Context.AnimationInstance, GetAnimation(), EndTime, WarpPointAnimBoneName);
					}
				}
			}

			// Update Target Transform based on the offset between the root and the warp point in the animation
			TargetTransform = CachedOffsetFromWarpPoint.GetValue() * WarpPointTransformGame;
		}

		if (!CachedTargetTransform.Equals(TargetTransform))
		{
			CachedTargetTransform = TargetTransform;

			OnTargetTransformChanged();
		}
	}	
}

void UGMCE_RootMotionModifier_Warp::OnTargetTransformChanged()
{
	if (AGMC_Pawn* PawnOwner = GetPawnOwner())
	{
		ActualStartTime = PreviousPosition;
		IGMCE_MotionWarpSubject* MotionWarpInterfacePawn = Cast<IGMCE_MotionWarpSubject>(PawnOwner);
		
		const float CapsuleHalfHeight = MotionWarpInterfacePawn->MotionWarping_GetCollisionHalfHeight();
		const FQuat CurrentRotation = PawnOwner->GetActorQuat();
		const FVector CurrentLocation = (PawnOwner->GetActorLocation() - CurrentRotation.GetUpVector() * CapsuleHalfHeight);
		StartTransform = FTransform(CurrentRotation, CurrentLocation);
	}	
}

FQuat UGMCE_RootMotionModifier_Warp::GetTargetRotation(const FGMCE_MotionWarpContext& Context) const
{
	if (RotationType == EGMCE_MotionWarpRotationType::Default)
	{
		return CachedTargetTransform.GetRotation();
	}
	else if (RotationType == EGMCE_MotionWarpRotationType::Facing)
	{
		const FTransform& CharacterTransform = Context.OwnerTransform;
		const FVector ToSyncPoint = (CachedTargetTransform.GetLocation() - CharacterTransform.GetLocation()).GetSafeNormal2D();
		return FRotationMatrix::MakeFromXZ(ToSyncPoint, FVector::UpVector).ToQuat();
	}

	return FQuat::Identity;	
}

FQuat UGMCE_RootMotionModifier_Warp::WarpRotation(const FGMCE_MotionWarpContext& WarpContext, const FTransform& RootMotionDelta, const FTransform& RootMotionTotal,
	float DeltaSeconds)
{
	const FQuat TotalRootMotionRotation = RootMotionTotal.GetRotation();
	const FQuat CurrentRotation = WarpContext.OwnerTransform.GetRotation() * WarpContext.MeshRelativeTransform.GetRotation();
	const FQuat TargetRotation = CurrentRotation.Inverse() * (GetTargetRotation(WarpContext) * WarpContext.MeshRelativeTransform.GetRotation());
	const float TimeRemaining = (EndTime - PreviousPosition) * WarpRotationTimeMultiplier;
	const float Alpha = FMath::Clamp(DeltaSeconds / TimeRemaining, 0.f, 1.f);
	FQuat TargetRotThisFrame = FQuat::Slerp(TotalRootMotionRotation, TargetRotation, Alpha);

	if (RotationMethod != EGMCE_MotionWarpRotationMethod::Slerp)
	{
		const float AngleDeltaThisFrame = TotalRootMotionRotation.AngularDistance(TargetRotThisFrame);
		const float MaxAngleDelta = FMath::Abs(FMath::DegreesToRadians(DeltaSeconds * WarpMaxRotationRate));
		const float TotalAngleDelta = TotalRootMotionRotation.AngularDistance(TargetRotation);
		if (RotationMethod == EGMCE_MotionWarpRotationMethod::ConstantRate && (TotalAngleDelta <= MaxAngleDelta))
		{
			TargetRotThisFrame = TargetRotation;
		}
		else if ((AngleDeltaThisFrame > MaxAngleDelta) || RotationMethod == EGMCE_MotionWarpRotationMethod::ConstantRate)
		{
			const FVector CrossProduct = FVector::CrossProduct(TotalRootMotionRotation.Vector(), TargetRotation.Vector());
			const float SignDirection = FMath::Sign(CrossProduct.Z);
			const FQuat ClampedRotationThisFrame = FQuat(FVector(0, 0, 1), MaxAngleDelta * SignDirection);
			TargetRotThisFrame = ClampedRotationThisFrame;
		}
	}

	const FQuat DeltaOut = TargetRotThisFrame * TotalRootMotionRotation.Inverse();
	
	return (DeltaOut * RootMotionDelta.GetRotation());
}

FString UGMCE_RootMotionModifier_Warp::DisplayString() const
{
	if (WarpTargetName != NAME_None && !WarpTargetName.ToString().IsEmpty()) return WarpTargetName.ToString();
	
	return Super::DisplayString();
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
void UGMCE_RootMotionModifier_Warp::PrintLog(const FString& Name, const FTransform& OriginalRootMotion, const FTransform& WarpedRootMotion) const
{
	if (const AGMC_Pawn* Pawn = GetPawnOwner())
	{
		UGMCE_OrganicMovementCmp* MovementCmp = Cast<UGMCE_OrganicMovementCmp>(Pawn->GetMovementComponent());
		USkeletalMeshComponent* MeshCmp = Pawn->GetComponentByClass<USkeletalMeshComponent>();
		
		const float CapsuleHalfHeight = MovementCmp->GetRootCollisionHalfHeight(true);
		const FVector CurrentLocation = (Pawn->GetActorLocation() - FVector(0.f, 0.f, CapsuleHalfHeight));
		const FVector CurrentToTarget = (GetTargetLocation() - CurrentLocation).GetSafeNormal2D();
		const FVector FutureLocation = CurrentLocation + (MeshCmp->ConvertLocalRootMotionToWorld(WarpedRootMotion)).GetTranslation();
		const FRotator CurrentRotation = Pawn->GetActorRotation();
		const FRotator FutureRotation = (WarpedRootMotion.GetRotation() * Pawn->GetActorQuat()).Rotator();
		const float Dot = FVector::DotProduct(Pawn->GetActorForwardVector(), CurrentToTarget);
		const float CurrentDist2D = FVector::Dist2D(GetTargetLocation(), CurrentLocation);
		const float FutureDist2D = FVector::Dist2D(GetTargetLocation(), FutureLocation);
		const float DeltaSeconds = Pawn->GetWorld()->GetDeltaSeconds();
		const float Speed = WarpedRootMotion.GetTranslation().Size() / DeltaSeconds;
		const float EndTimeOffset = CurrentPosition - EndTime;

		UE_LOG(LogGMCExAnimation, Log, TEXT("%s NetMode: %d Char: %s Anim: %s Win: [%f %f][%f %f] DT: %f WT: %f ETOffset: %f Dist2D: %f Z: %f FDist2D: %f FZ: %f Dot: %f Delta: %s (%f) FDelta: %s (%f) Speed: %f Loc: %s FLoc: %s Rot: %s FRot: %s"),
			*Name, (int32)Pawn->GetWorld()->GetNetMode(), *GetNameSafe(Pawn), *GetNameSafe(AnimationSequence.Get()), StartTime, EndTime, PreviousPosition, CurrentPosition, DeltaSeconds, Pawn->GetWorld()->GetTimeSeconds(), EndTimeOffset,
			CurrentDist2D, (GetTargetLocation().Z - CurrentLocation.Z), FutureDist2D, (GetTargetLocation().Z - FutureLocation.Z), Dot,
			*OriginalRootMotion.GetTranslation().ToString(), OriginalRootMotion.GetTranslation().Size(), *WarpedRootMotion.GetTranslation().ToString(), WarpedRootMotion.GetTranslation().Size(), Speed,
			*CurrentLocation.ToString(), *FutureLocation.ToString(), *CurrentRotation.ToCompactString(), *FutureRotation.ToCompactString());
	}
}
#endif