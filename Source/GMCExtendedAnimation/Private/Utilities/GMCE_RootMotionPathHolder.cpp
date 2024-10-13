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
	int32 NumSamples = static_cast<int32>((Montage->GetPlayLength() - InContext.CurrentPosition) / SampleSize) + 1;
	float CurrentTime = InContext.CurrentPosition;

	float PredictionTime = 0.f;
	float LastSample = 0.f;

	FGMCE_MovementSampleCollection PredictionSamples;
	PredictionSamples.Samples.Reserve(NumSamples);

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

		PredictionTime += WarpContext.DeltaSeconds;

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

		FTransform FlattenedTransform = CurrentWorldTransform;
		FlattenedTransform.SetTranslation(FlattenedTransform.GetTranslation() + InContext.MeshRelativeTransform.GetTranslation());

		if (PredictionTime - LastSample > 0.01f)
		{
			FGMCE_MovementSample NewSample;
			NewSample.AccumulatedSeconds = PredictionTime;
			NewSample.WorldTransform = FlattenedTransform;
			NewSample.WorldLinearVelocity = (CurrentWorldTransform.GetLocation() - PreviousWorldTransform.GetLocation()) / (PredictionTime - LastSample);
			NewSample.ActorWorldRotation = CurrentWorldTransform.GetRotation().Rotator();
			NewSample.ActorWorldTransform = CurrentWorldTransform;

			PredictionSamples.Samples.Add(NewSample);
			LastSample = PredictionTime;
		}

		PreviousTimestamp = CurrentTime;
		PreviousWorldTransform = CurrentWorldTransform;
	}

	PredictionSamples.DrawDebug(WarpingComponent->GetWorld(), InContext.OwnerTransform, FColor::Red, FColor::White, FColor::Purple, 0, Montage->GetPlayLength() - InContext.CurrentPosition);
	
	return false;
}

void UGMCE_RootMotionPathHolder::TestGeneratePath(AGMC_Pawn* Pawn, UAnimMontage* Montage,
	float StartPosition, float PlayRate)
{
	IGMCE_MotionWarpSubject* WarpingSubject = Cast<IGMCE_MotionWarpSubject>(Pawn);
	UGMCE_OrganicMovementCmp* MovementComponent = WarpingSubject->GetGMCExMovementComponent();

	FTransform InitialTransform = MovementComponent->GetActorTransform_GMC();
	// FRotator CurrentMeshRotation = MovementComponent->GetSkeletalMeshReference()->GetRelativeRotation();
	// InitialTransform.SetRotation((CurrentMeshRotation + FRotator(0.f, 90.f, 0.f)).Quaternion());
	
	FGMCE_MotionWarpContext WarpContext;
	WarpContext.Animation = Montage;
	WarpContext.OwnerTransform = InitialTransform;
	WarpContext.MeshRelativeTransform = FTransform(WarpingSubject->MotionWarping_GetRotationOffset(), WarpingSubject->MotionWarping_GetTranslationOffset());
	WarpContext.AnimationInstance = MovementComponent->GetSkeletalMeshReference()->GetAnimInstance();
	WarpContext.PlayRate = PlayRate;
	WarpContext.CurrentPosition = StartPosition;
	WarpContext.PreviousPosition = StartPosition;
	WarpContext.Weight = 1.f;

	UGMCE_MotionWarpingComponent *WarpComponent = Cast<UGMCE_MotionWarpingComponent>(MovementComponent->GetOwner()->GetComponentByClass(UGMCE_MotionWarpingComponent::StaticClass()));	
	
	GeneratePathForMontage(WarpComponent, WarpingSubject->MotionWarping_GetMeshComponent(), Montage, WarpContext);
}
