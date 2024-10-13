// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/GMCE_RootMotionPathHolder.h"

#include "GMCExtendedAnimationLog.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_MotionWarpingUtilities.h"

bool UGMCE_RootMotionPathHolder::GeneratePathForMontage(UGMCE_MotionWarpingComponent* WarpingComponent, USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage, const FGMCE_MotionWarpContext& InContext)
{
	if (Montage->GetNumberOfSampledKeys() < 1) return false;

	// FTransform FinalTransform = UGMCE_MotionWarpingUtilities::ExtractRootTransformFromAnimation(Montage, InContext.CurrentPosition);

	float PreviousTimestamp = InContext.CurrentPosition;
	FTransform CurrentWorldTransform = InContext.OwnerTransform;
	FTransform PreviousWorldTransform = InContext.OwnerTransform;

	const float SampleSize = (Montage->GetPlayLength() / (Montage->GetNumberOfSampledKeys() * 2.f)) / InContext.PlayRate;
	float CurrentTime = InContext.CurrentPosition;

	while (CurrentTime < Montage->GetPlayLength())
	{
		CurrentTime = FMath::Min(CurrentTime + SampleSize, Montage->GetPlayLength());
		
		const FTransform RawMovement = UGMCE_MotionWarpingUtilities::ExtractRootMotionFromAnimation(Montage, PreviousTimestamp, CurrentTime);

		FGMCE_MotionWarpContext WarpContext = InContext;
		WarpContext.CurrentPosition = CurrentTime;
		WarpContext.PreviousPosition = PreviousTimestamp;
		WarpContext.DeltaSeconds = CurrentTime - PreviousTimestamp;
		WarpContext.OwnerTransform = CurrentWorldTransform;
		const FTransform NewMovement = WarpingComponent->ProcessRootMotionFromContext(RawMovement, WarpContext);

		//Calculate new actor transform after applying root motion to this component
		const FTransform ActorToWorld = CurrentWorldTransform;
		const FTransform ComponentTransform = FTransform(InContext.MeshRelativeTransform.GetRotation().Rotator(), InContext.MeshRelativeTransform.GetTranslation()) * ActorToWorld;
		const FTransform ComponentToActor = InContext.MeshRelativeTransform.Inverse();

		const FTransform NewComponentToWorld = NewMovement * ComponentTransform;
		const FTransform NewActorTransform = ComponentToActor * NewComponentToWorld;

		const FVector DeltaWorldTranslation = NewActorTransform.GetTranslation() - ActorToWorld.GetTranslation();

		const FQuat NewWorldRotation = ComponentTransform.GetRotation() * NewMovement.GetRotation();
		const FQuat DeltaWorldRotation = NewWorldRotation * ComponentTransform.GetRotation().Inverse();
	
		const FTransform DeltaWorldTransform(DeltaWorldRotation, DeltaWorldTranslation);
		
		CurrentWorldTransform.Accumulate(DeltaWorldTransform);

		const FVector LineStart = PreviousWorldTransform.GetLocation() + WarpContext.MeshRelativeTransform.GetTranslation();
		const FVector LineEnd = CurrentWorldTransform.GetLocation() + WarpContext.MeshRelativeTransform.GetTranslation();
		
		DrawDebugLine(WarpingComponent->GetWorld(), LineStart, LineEnd, FColor::Yellow, false, 2.f, 0, 1.f);

		PreviousTimestamp = CurrentTime;
		PreviousWorldTransform = CurrentWorldTransform;
	}
	
	return false;
}

void UGMCE_RootMotionPathHolder::TestGeneratePath(AGMC_Pawn* Pawn, UAnimMontage* Montage,
	float StartPosition, float PlayRate)
{
	IGMCE_MotionWarpSubject* WarpingSubject = Cast<IGMCE_MotionWarpSubject>(Pawn);
	UGMCE_OrganicMovementCmp* MovementComponent = WarpingSubject->GetGMCExMovementComponent();
	
	FGMCE_MotionWarpContext WarpContext;
	WarpContext.Animation = Montage;
	WarpContext.OwnerTransform = MovementComponent->GetActorTransform_GMC();
	WarpContext.MeshRelativeTransform = MovementComponent->GetSkeletalMeshReference()->GetRelativeTransform();
	WarpContext.AnimationInstance = MovementComponent->GetSkeletalMeshReference()->GetAnimInstance();
	WarpContext.PlayRate = PlayRate;
	WarpContext.CurrentPosition = StartPosition;
	WarpContext.PreviousPosition = StartPosition;
	WarpContext.Weight = 1.f;

	UGMCE_MotionWarpingComponent *WarpComponent = Cast<UGMCE_MotionWarpingComponent>(MovementComponent->GetOwner()->GetComponentByClass(UGMCE_MotionWarpingComponent::StaticClass()));	
	
	GeneratePathForMontage(WarpComponent, WarpingSubject->MotionWarping_GetMeshComponent(), Montage, WarpContext);
}
