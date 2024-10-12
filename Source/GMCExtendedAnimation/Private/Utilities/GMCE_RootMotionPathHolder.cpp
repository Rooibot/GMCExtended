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
	FTransform LastWorldTransform = InContext.OwnerTransform;

	UE_LOG(LogGMCExAnimation, Log, TEXT("Starting at %s"), *LastWorldTransform.GetLocation().ToCompactString())
	
	for(int32 Idx = 1; Idx < Montage->GetNumberOfSampledKeys(); Idx++)
	{
		float CurrentTime = Montage->GetTimeAtFrame(Idx);
		if (CurrentTime < PreviousTimestamp) continue;

		FTransform NewMovement = UGMCE_MotionWarpingUtilities::ExtractRootMotionFromAnimation(Montage, PreviousTimestamp, CurrentTime);

		FGMCE_MotionWarpContext WarpContext = InContext;
		WarpContext.CurrentPosition = CurrentTime;
		WarpContext.PreviousPosition = PreviousTimestamp;
		WarpContext.DeltaSeconds = CurrentTime - PreviousTimestamp;
		WarpContext.OwnerTransform = LastWorldTransform;
		NewMovement = WarpingComponent->ProcessRootMotionFromContext(NewMovement, WarpContext);
 
		//Calculate new actor transform after applying root motion to this component
		const FTransform ActorToWorld = LastWorldTransform;
		const FTransform ComponentTransform = WarpContext.MeshRelativeTransform * ActorToWorld;
		const FTransform ComponentToActor = ActorToWorld.GetRelativeTransform(ComponentTransform);

		const FTransform NewComponentToWorld = NewMovement * ComponentTransform;
		const FTransform NewActorTransform = ComponentToActor * NewComponentToWorld;

		const FVector DeltaWorldTranslation = NewActorTransform.GetTranslation() - ActorToWorld.GetTranslation();

		const FQuat NewWorldRotation = ComponentTransform.GetRotation() * NewMovement.GetRotation();
		const FQuat DeltaWorldRotation = NewWorldRotation * ComponentTransform.GetRotation().Inverse();
	
		const FTransform DeltaWorldTransform(DeltaWorldRotation, DeltaWorldTranslation);
		
		LastWorldTransform.Accumulate(DeltaWorldTransform);
		DrawDebugSphere(WarpingComponent->GetWorld(), LastWorldTransform.GetLocation(), 10, 8, FColor::Yellow, false, 2.f, 0, 1.f);
		UE_LOG(LogGMCExAnimation, Log, TEXT("Warped location: [%f - %f] %s -> %s"),
			PreviousTimestamp, CurrentTime, *DeltaWorldTransform.GetTranslation().ToCompactString(),
			*LastWorldTransform.GetLocation().ToCompactString())
		PreviousTimestamp = CurrentTime;
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
	WarpContext.PlayRate = PlayRate;
	WarpContext.CurrentPosition = StartPosition;
	WarpContext.PreviousPosition = StartPosition;
	WarpContext.Weight = 1.f;

	UGMCE_MotionWarpingComponent *WarpComponent = Cast<UGMCE_MotionWarpingComponent>(MovementComponent->GetOwner()->GetComponentByClass(UGMCE_MotionWarpingComponent::StaticClass()));	
	
	GeneratePathForMontage(WarpComponent, WarpingSubject->MotionWarping_GetMeshComponent(), Montage, WarpContext);
}
