// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/GMCE_RootMotionPathHolder.h"

#include "GMCExtendedAnimationLog.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_MotionWarpingUtilities.h"
#include "DSP/SampleBufferReader.h"

bool UGMCE_RootMotionPathHolder::GeneratePathForMontage(UGMCE_MotionWarpingComponent* WarpingComponent, USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage, const FGMCE_MotionWarpContext& InContext)
{
	if (Montage->GetNumberOfSampledKeys() < 1) return false;

	float PreviousTimestamp = InContext.CurrentPosition;
	FTransform CurrentWorldTransform = InContext.OwnerTransform;
	FTransform PreviousWorldTransform = InContext.OwnerTransform;

	const float SampleSize = (Montage->GetPlayLength() / (Montage->GetNumberOfSampledKeys() * 4.f)) / InContext.PlayRate;
	int32 NumSamples = static_cast<int32>(Montage->GetPlayLength() / PredictionSampleInterval) + 1;
	float CurrentTime = InContext.CurrentPosition;

	float LastSample = 0.f;

	FGMCE_MovementSampleCollection PredictedPathSamples;
	PredictedPathSamples.Samples.Reserve(NumSamples);

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

		FTransform FlattenedTransform = CurrentWorldTransform;
		FlattenedTransform.SetTranslation(FlattenedTransform.GetTranslation() + InContext.MeshRelativeTransform.GetTranslation());

		if (CurrentTime - LastSample > PredictionSampleInterval)
		{
			FGMCE_MovementSample NewSample;
			NewSample.AccumulatedSeconds = CurrentTime;
			NewSample.WorldTransform = FlattenedTransform;
			NewSample.WorldLinearVelocity = (CurrentWorldTransform.GetLocation() - PreviousWorldTransform.GetLocation()) / (CurrentTime - LastSample);
			NewSample.ActorWorldRotation = CurrentWorldTransform.GetRotation().Rotator();
			NewSample.ActorWorldTransform = CurrentWorldTransform;

			PredictedPathSamples.Samples.Add(NewSample);
			LastSample = CurrentTime;
		}

		PreviousTimestamp = CurrentTime;
		PreviousWorldTransform = CurrentWorldTransform;
	}

	PredictedSamples = PredictedPathSamples;
	
	return !PredictedSamples.Samples.IsEmpty();
}

void UGMCE_RootMotionPathHolder::GenerateMontagePath(AGMC_Pawn* Pawn, UAnimMontage* Montage,
	float StartPosition, float PlayRate, bool bShowDebug)
{
	IGMCE_MotionWarpSubject* WarpingSubject = Cast<IGMCE_MotionWarpSubject>(Pawn);
	UGMCE_OrganicMovementCmp* MovementComponent = WarpingSubject->GetGMCExMovementComponent();

	const FTransform InitialTransform = MovementComponent->GetActorTransform_GMC();
	const FTransform MeshTransform = FTransform(WarpingSubject->MotionWarping_GetRotationOffset(), WarpingSubject->MotionWarping_GetTranslationOffset());
	
	GenerateMontagePathWithOverrides(Pawn, Montage, StartPosition, PlayRate, InitialTransform, MeshTransform, bShowDebug);
}

void UGMCE_RootMotionPathHolder::GenerateMontagePathWithOverrides(AGMC_Pawn* Pawn, UAnimMontage* Montage,
	float StartPosition, float PlayRate, const FTransform& OriginTransform, const FTransform& MeshRelativeTransform,
	bool bDrawDebug)
{
	IGMCE_MotionWarpSubject* WarpingSubject = Cast<IGMCE_MotionWarpSubject>(Pawn);
	UGMCE_OrganicMovementCmp* MovementComponent = WarpingSubject->GetGMCExMovementComponent();

	FTransform InitialTransform = MovementComponent->GetActorTransform_GMC();
	
	FGMCE_MotionWarpContext WarpContext;
	WarpContext.Animation = Montage;
	WarpContext.OwnerTransform = OriginTransform;
	WarpContext.MeshRelativeTransform = MeshRelativeTransform;
	WarpContext.AnimationInstance = MovementComponent->GetSkeletalMeshReference()->GetAnimInstance();
	WarpContext.PlayRate = PlayRate;
	WarpContext.CurrentPosition = StartPosition;
	WarpContext.PreviousPosition = StartPosition;
	WarpContext.Weight = 1.f;

	UGMCE_MotionWarpingComponent *WarpComponent = Cast<UGMCE_MotionWarpingComponent>(MovementComponent->GetOwner()->GetComponentByClass(UGMCE_MotionWarpingComponent::StaticClass()));	
	
	if (GeneratePathForMontage(WarpComponent, WarpingSubject->MotionWarping_GetMeshComponent(), Montage, WarpContext))
	{
		if (bDrawDebug)
		{
			FColor PredictionColor = FColor::Green;
			if (MovementComponent->IsRemotelyControlledServerPawn())
			{
				PredictionColor = FColor::Black;
			}
			PredictedSamples.DrawDebug(MovementComponent->GetWorld(), WarpContext.OwnerTransform, PredictionColor, PredictionColor, FColor::Red, 0, 1.f);
		}
	}

	FGMCE_MovementSample FirstSample = PredictedSamples.Samples[0];
	FGMCE_MovementSample LastSample = PredictedSamples.Samples.Last();
	
	UE_LOG(LogGMCExAnimation, Log, TEXT("[%s] PATH: Starts at %f from %s %s, mesh relative %s."),
		*MovementComponent->GetComponentDescription(), FirstSample.AccumulatedSeconds, *FirstSample.WorldTransform.GetLocation().ToCompactString(), *FirstSample.WorldTransform.GetRotation().Rotator().ToCompactString(), *MeshRelativeTransform.ToString());
	UE_LOG(LogGMCExAnimation, Log, TEXT("[%s] PATH:   Ends at %f from %s %s."),
			*MovementComponent->GetComponentDescription(), LastSample.AccumulatedSeconds, *LastSample.WorldTransform.GetLocation().ToCompactString(), *LastSample.WorldTransform.GetRotation().Rotator().ToCompactString());

	for (const auto& Target : WarpComponent->GetWarpTargets().GetTargets())
	{
		UE_LOG(LogGMCExAnimation, Log, TEXT("[%s] PATH: Target %s: %s %s"),
			*MovementComponent->GetComponentDescription(), *Target.Name.ToString(), *Target.Location.ToCompactString(), *Target.Rotation.ToCompactString());
	}
		UE_LOG(LogGMCExAnimation, Log, TEXT("[%s] --------------"), *MovementComponent->GetComponentDescription())
}

FTransform UGMCE_RootMotionPathHolder::GetTransformAtPosition(float Position)
{
	const FGMCE_MovementSample FoundSample = PredictedSamples.GetSampleAtTime(Position, true);

	return FoundSample.WorldTransform;
}

void UGMCE_RootMotionPathHolder::Reset()
{
	PredictedSamples.Samples.Empty();
}

void UGMCE_RootMotionPathHolder::GetDeltaBetweenPositions(float StartPosition, float EndPosition, FVector& OutDelta, FVector& OutVelocity, float DeltaTimeOverride = -1.f)
{
	const FGMCE_MovementSample FirstSample = PredictedSamples.GetSampleAtTime(StartPosition, true);
	const FGMCE_MovementSample SecondSample = PredictedSamples.GetSampleAtTime(EndPosition, true);

	const float Time = DeltaTimeOverride > 0.f ? DeltaTimeOverride : (EndPosition - StartPosition);
	
	const FVector Delta = SecondSample.WorldTransform.GetLocation() - FirstSample.WorldTransform.GetLocation();
	const FVector Velocity = Time > 0.f ? Delta / Time : FVector::ZeroVector;

	OutDelta = Delta;
	OutVelocity = Velocity;
}
