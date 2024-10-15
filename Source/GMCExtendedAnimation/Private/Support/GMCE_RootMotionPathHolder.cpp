// Fill out your copyright notice in the Description page of Project Settings.


#include "GMCE_RootMotionPathHolder.h"
#include "AnimNotifyState_GMCExEarlyBlendOut.h"
#include "GMCExtendedAnimationLog.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_MotionWarpingUtilities.h"

bool UGMCE_RootMotionPathHolder::GeneratePathForMontage(UGMCE_MotionWarpingComponent* WarpingComponent, USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage, const FGMCE_MotionWarpContext& InContext)
{
	Reset();
	
	if (Montage->GetNumberOfSampledKeys() < 1) return false;
	
	float PreviousTimestamp = InContext.CurrentPosition;
	FTransform CurrentWorldTransform = InContext.OwnerTransform;
	FTransform PreviousWorldTransform = InContext.OwnerTransform;

	const float SampleSize = (Montage->GetPlayLength() / (Montage->GetNumberOfSampledKeys() * 10.f)) / InContext.PlayRate;
	int32 NumSamples = static_cast<int32>(Montage->GetPlayLength() / PredictionSampleInterval) + 1;
	float CurrentTime = InContext.CurrentPosition;

	float LastSample = 0.f;

	FGMCE_MovementSampleCollection PredictedPathSamples;
	PredictedPathSamples.Samples.Reserve(NumSamples);
	PredictionSequence = Montage;

	FTransform FlattenedTransform = CurrentWorldTransform;
	FlattenedTransform.SetTranslation(FlattenedTransform.GetTranslation() + InContext.MeshRelativeTransform.GetTranslation());
	FlattenedTransform.SetRotation((FlattenedTransform.GetRotation().Rotator() + InContext.MeshRelativeTransform.GetRotation().Rotator()).Quaternion());
	FTransform PreviousSampledTransform = FlattenedTransform;
	
	FGMCE_MovementSample NewSample;
	NewSample.AccumulatedSeconds = CurrentTime;
	NewSample.WorldTransform = FlattenedTransform;
	NewSample.ActorWorldRotation = CurrentWorldTransform.GetRotation().Rotator();
	NewSample.ActorWorldTransform = CurrentWorldTransform;
	
	PredictedPathSamples.Samples.Add(NewSample);
	LastSample = CurrentTime;

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

		if (!NewMovement.Equals(FTransform::Identity))
		{
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

			FlattenedTransform = CurrentWorldTransform;
			FlattenedTransform.SetTranslation(FlattenedTransform.GetTranslation() + InContext.MeshRelativeTransform.GetTranslation());
			FlattenedTransform.SetRotation((FlattenedTransform.GetRotation().Rotator() + InContext.MeshRelativeTransform.GetRotation().Rotator()).Quaternion());
		}

		if (CurrentTime - LastSample > PredictionSampleInterval)
		{
			NewSample = FGMCE_MovementSample();
			NewSample.AccumulatedSeconds = CurrentTime;
			NewSample.WorldTransform = FlattenedTransform;
			NewSample.WorldLinearVelocity = (FlattenedTransform.GetLocation() - PreviousSampledTransform.GetLocation()) / (CurrentTime - LastSample);
			NewSample.ActorWorldRotation = CurrentWorldTransform.GetRotation().Rotator();
			NewSample.ActorWorldTransform = CurrentWorldTransform;
			NewSample.ComponentLocalTransform = NewMovement;

			PredictedPathSamples.Samples.Add(NewSample);
			LastSample = CurrentTime;

			PreviousSampledTransform = FlattenedTransform;
			PreviousWorldTransform = CurrentWorldTransform;
		}

		PreviousTimestamp = CurrentTime;
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
}

bool UGMCE_RootMotionPathHolder::GetTransformsAtPosition(float Position, FTransform& OutComponentTransform, FTransform& OutActorTransform)
{
	if (PredictedSamples.Samples.IsEmpty())
	{
		return false;
	}

	if (Position < PredictedSamples.Samples[0].AccumulatedSeconds || Position > PredictedSamples.Samples.Last().AccumulatedSeconds)
	{
		return false;
	}
	
	const FGMCE_MovementSample FoundSample = PredictedSamples.GetSampleAtTime(Position, true);

	OutComponentTransform = FoundSample.WorldTransform;
	OutActorTransform = FoundSample.ActorWorldTransform;

	return true;
}

bool UGMCE_RootMotionPathHolder::GetTransformsAtPositionWithBlendOut(const UGMCE_OrganicMovementCmp* MovementComponent,
	float Position, FTransform& OutComponentTransform, FTransform& OutActorTransform, bool &OutShouldBlendOut, float &OutBlendOutTime)
{
	OutShouldBlendOut = false;
	
	if (!MovementComponent)
	{
		return GetTransformsAtPosition(Position, OutComponentTransform, OutActorTransform);
	}

	if (PredictionSequence)
	{
		for (const auto& Notify : PredictionSequence->Notifies)
		{
			if (Position > Notify.GetTriggerTime() && Position < Notify.GetEndTriggerTime())
			{
				const UAnimNotifyState_GMCExEarlyBlendOut* BlendOutNotify = Notify.NotifyStateClass ? Cast<UAnimNotifyState_GMCExEarlyBlendOut>(Notify.NotifyStateClass) : nullptr;
				if (BlendOutNotify && BlendOutNotify->ShouldBlendOut(MovementComponent, true))
				{
					OutBlendOutTime = BlendOutNotify->BlendOutTime;
					OutShouldBlendOut = true;
					break;
				}
			}
		}
	}

	const bool bResult = GetTransformsAtPosition(Position, OutComponentTransform, OutActorTransform);

	if (!bResult)
	{
		UE_LOG(LogGMCExAnimation, Verbose, TEXT("[%s] Got bad result for %f with %d samples."), *MovementComponent->GetComponentDescription(), Position, PredictedSamples.Samples.Num())
	}
	
	return bResult;
}

void UGMCE_RootMotionPathHolder::Reset()
{
	PredictedSamples = FGMCE_MovementSampleCollection();
	PredictionSequence = nullptr;
	CachedPredictedBlendOut = -1.f;
}

void UGMCE_RootMotionPathHolder::GetActorDeltaBetweenPositions(float StartPosition, float EndPosition, const FVector& OverrideOrigin, FVector& OutDelta, FVector& OutVelocity, float DeltaTimeOverride = -1.f, bool bShowDebug = false)
{
	if (PredictedSamples.Samples.IsEmpty() || EndPosition < StartPosition || StartPosition < PredictedSamples.Samples[0].AccumulatedSeconds)
	{
		OutDelta = FVector::ZeroVector;
		OutVelocity = FVector::ZeroVector;
		return;
	}

	const FGMCE_MovementSample FirstSample = PredictedSamples.GetSampleAtTime(StartPosition, true);
	const FGMCE_MovementSample SecondSample = PredictedSamples.GetSampleAtTime(EndPosition, true);
	
	const float Time = DeltaTimeOverride > 0.f ? DeltaTimeOverride : (SecondSample.AccumulatedSeconds - FirstSample.AccumulatedSeconds);

	const FVector StartLocation = OverrideOrigin.IsZero() ? FirstSample.ActorWorldTransform.GetLocation() : OverrideOrigin;
	
	const FVector Delta = SecondSample.ActorWorldTransform.GetLocation() - StartLocation;
	const FVector Velocity = Time > 0.f ? Delta / Time : FVector::ZeroVector;
	OutDelta = Delta;
	OutVelocity = Velocity;

	if (bShowDebug)
	{
		APawn* PawnTest = Cast<APawn>(GetOuter());
		FColor DebugColor = FColor::Silver;
		FColor DeviationColor = FColor::Red;
		if (PawnTest && PawnTest->HasAuthority())
		{
			DebugColor = FColor::Purple;
			DeviationColor = FColor::Orange;
		}
	
		DrawDebugLine(GetOuter()->GetWorld(), StartLocation, SecondSample.ActorWorldTransform.GetLocation(), DebugColor, false, 1.f, 0, 1.f);
		if (!OverrideOrigin.IsZero() && (OverrideOrigin - FirstSample.ActorWorldTransform.GetLocation()).Length() > 2.f)
		{
			DrawDebugPoint(GetOuter()->GetWorld(), OverrideOrigin, 4.f, DeviationColor, false, 1.f);
			DrawDebugLine(GetOuter()->GetWorld(), OverrideOrigin, FirstSample.ActorWorldTransform.GetLocation(), DeviationColor, false, 1.f, 0, 1.f);
			DrawDebugLine(GetOuter()->GetWorld(), FirstSample.ActorWorldTransform.GetLocation(), SecondSample.ActorWorldTransform.GetLocation(), DebugColor, false, 1.f, 0, 1.f);
		}
	}

}

bool UGMCE_RootMotionPathHolder::GetActorDeltaBetweenPositionsWithBlendOut(const UGMCE_OrganicMovementCmp* MovementComponent, float StartPosition, float EndPosition,
	const FVector& OverrideOrigin, FVector& OutDelta, FVector& OutVelocity, float &OutBlendOutTime, float DeltaTimeOverride, bool bShowDebug)
{
	bool bShouldBlendOut = false;
	if (PredictionSequence)
	{
		for (const auto& Notify : PredictionSequence->Notifies)
		{
			if ((StartPosition > Notify.GetTriggerTime() && StartPosition < Notify.GetEndTriggerTime()) || (EndPosition > Notify.GetEndTriggerTime() && EndPosition < Notify.GetEndTriggerTime()))
			{
				const UAnimNotifyState_GMCExEarlyBlendOut* BlendOutNotify = Notify.NotifyStateClass ? Cast<UAnimNotifyState_GMCExEarlyBlendOut>(Notify.NotifyStateClass) : nullptr;
				if (BlendOutNotify)
				{
					bShouldBlendOut = BlendOutNotify->ShouldBlendOut(MovementComponent, true);
					if (bShouldBlendOut)
					{
						OutBlendOutTime = BlendOutNotify->BlendOutTime;
						break;
					}
				}
			}
		}
	}

	if (bShouldBlendOut)
	{
		OutDelta = FVector::ZeroVector;
		OutVelocity = FVector::ZeroVector;
		return true;
	}
	
	GetActorDeltaBetweenPositions(StartPosition, EndPosition, OverrideOrigin, OutDelta, OutVelocity, DeltaTimeOverride, bShowDebug);
	return false;
}

bool UGMCE_RootMotionPathHolder::GetActorDeltaTransformBetweenPositionsWithBlendOut(
	const UGMCE_OrganicMovementCmp* MovementComponent, float StartPosition, float EndPosition,
	const FTransform& OverrideOrigin, FTransform& OutDelta, float& OutBlendOutTime, float DeltaTimeOverride,
	bool bShowDebug)
{
	bool bShouldBlendOut = false;
	OutBlendOutTime = 0.f;
	if (PredictionSequence)
	{
		for (const auto& Notify : PredictionSequence->Notifies)
		{
			if ((StartPosition > Notify.GetTriggerTime() && StartPosition < Notify.GetEndTriggerTime()) || (EndPosition > Notify.GetEndTriggerTime() && EndPosition < Notify.GetEndTriggerTime()))
			{
				const UAnimNotifyState_GMCExEarlyBlendOut* BlendOutNotify = Notify.NotifyStateClass ? Cast<UAnimNotifyState_GMCExEarlyBlendOut>(Notify.NotifyStateClass) : nullptr;
				if (BlendOutNotify)
				{
					bShouldBlendOut = BlendOutNotify->ShouldBlendOut(MovementComponent, true);
					if (bShouldBlendOut)
					{
						OutBlendOutTime = BlendOutNotify->BlendOutTime;
						break;
					}
				}
			}
		}
	}

	if (bShouldBlendOut)
	{
		OutDelta = FTransform::Identity;
		return true;
	}

	if (PredictedSamples.Samples.IsEmpty() || EndPosition < StartPosition || StartPosition < PredictedSamples.Samples[0].AccumulatedSeconds)
	{
		OutDelta = FTransform::Identity;
		return false;
	}

	const FGMCE_MovementSample FirstSample = PredictedSamples.GetSampleAtTime(StartPosition, true);
	const FGMCE_MovementSample SecondSample = PredictedSamples.GetSampleAtTime(EndPosition, true);
	
	const float Time = DeltaTimeOverride > 0.f ? DeltaTimeOverride : (SecondSample.AccumulatedSeconds - FirstSample.AccumulatedSeconds);

	const FTransform StartTransform = OverrideOrigin.Equals(FTransform::Identity) ? FirstSample.ActorWorldTransform : OverrideOrigin;

	FRotator DeltaRotator = SecondSample.ActorWorldTransform.GetRotation().Rotator() - StartTransform.GetRotation().Rotator();
	FVector DeltaTranslation = SecondSample.ActorWorldTransform.GetLocation() - StartTransform.GetLocation();

	OutDelta = FTransform(DeltaRotator, DeltaTranslation);
	return false;
}

bool UGMCE_RootMotionPathHolder::GetPredictedPositionForBlendOut(float& OutBlendPosition)
{
	if (CachedPredictedBlendOut > 0.f)
	{
		OutBlendPosition = CachedPredictedBlendOut;
		return true;
	}

	bool bFound = false;
	if (PredictionSequence)
	{
		for (const auto& Notify : PredictionSequence->Notifies)
		{
			const UAnimNotifyState_GMCExEarlyBlendOut* BlendOutNotify = Notify.NotifyStateClass ? Cast<UAnimNotifyState_GMCExEarlyBlendOut>(Notify.NotifyStateClass) : nullptr;
			if (BlendOutNotify && (Notify.GetTriggerTime() < CachedPredictedBlendOut || CachedPredictedBlendOut < 0.f))
			{
				CachedPredictedBlendOut = Notify.GetTriggerTime();
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		CachedPredictedBlendOut = PredictedSamples.Samples.Last().AccumulatedSeconds;
	}
	
	OutBlendPosition = CachedPredictedBlendOut;
	return CachedPredictedBlendOut > 0.f;
}

bool UGMCE_RootMotionPathHolder::GetLinearVelocityAtPosition(float Position, FVector& OutVelocity)
{
	if (PredictedSamples.Samples.IsEmpty()) return false;

	if (Position < PredictedSamples.Samples[0].AccumulatedSeconds || Position > PredictedSamples.Samples.Last().AccumulatedSeconds) return false;

	FGMCE_MovementSample Sample = PredictedSamples.GetSampleAtTime(Position, true);
	OutVelocity = Sample.WorldLinearVelocity;
	return true;
}

bool UGMCE_RootMotionPathHolder::GetSampleRange(float& OutFirstSample, float& OutLastSample) const
{
	if (PredictedSamples.Samples.IsEmpty()) return false;
	
	OutFirstSample = PredictedSamples.Samples[0].AccumulatedSeconds;
	OutLastSample = PredictedSamples.Samples.Last().AccumulatedSeconds;
	return true;
}

FString UGMCE_RootMotionPathHolder::ToString() const
{
	if (PredictedSamples.Samples.IsEmpty()) return "empty";
	
	FString Result = FString::Printf(TEXT("%d samples: %f [%s] -> %f [%s]"),
		PredictedSamples.Samples.Num(), PredictedSamples.Samples[0].AccumulatedSeconds, *PredictedSamples.Samples[0].ActorWorldTransform.GetLocation().ToCompactString(),
		PredictedSamples.Samples.Last().AccumulatedSeconds, *PredictedSamples.Samples.Last().ActorWorldTransform.GetLocation().ToString());

	return Result;
}

bool UGMCE_RootMotionPathHolder::GetSampleAtPositionWithBlendOut(UGMCE_OrganicMovementCmp* MovementComponent, const float Position, FGMCE_MovementSample& OutSample,
	bool& bOutWantsBlend, float& OutBlendTime, bool bExtrapolate) const
{
	if (PredictedSamples.Samples.IsEmpty()) return false;

	if (Position < PredictedSamples.Samples[0].AccumulatedSeconds || Position > PredictedSamples.Samples.Last().AccumulatedSeconds) return false;
	
	if (PredictionSequence)
	{
		for (const auto& Notify : PredictionSequence->Notifies)
		{
			if (Position > Notify.GetTriggerTime() && Position < Notify.GetEndTriggerTime())
			{
				const UAnimNotifyState_GMCExEarlyBlendOut* BlendOutNotify = Notify.NotifyStateClass ? Cast<UAnimNotifyState_GMCExEarlyBlendOut>(Notify.NotifyStateClass) : nullptr;
				if (BlendOutNotify && BlendOutNotify->ShouldBlendOut(MovementComponent, true))
				{
					OutBlendTime = BlendOutNotify->BlendOutTime;
					bOutWantsBlend = true;
					break;
				}
			}
		}
	}
	
	OutSample = GetSampleForTime(Position, bExtrapolate);

	return true;
}
