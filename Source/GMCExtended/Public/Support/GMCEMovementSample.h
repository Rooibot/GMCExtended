#pragma once

#include "CoreMinimal.h"
#include "GMCExtendedLog.h"
#include "Animation/MotionTrajectoryTypes.h"
#include "Animation/TrajectoryTypes.h"
#include "PoseSearch/PoseSearchTrajectoryTypes.h"
#include "GMCEMovementSample.generated.h"

UENUM()
enum class EGMCE_TrajectoryRotationType : uint8
{
	Actor,
	Component,
	Controller,
	MeshOffset,
	Travel
};

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FGMCE_MovementSample
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	float AccumulatedSeconds { 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FTransform RelativeTransform { FTransform::Identity };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FVector RelativeLinearVelocity { 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FTransform WorldTransform { FTransform::Identity };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FVector WorldLinearVelocity { 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FRotator ActorWorldRotation { FRotator::ZeroRotator };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FTransform ActorWorldTransform { FTransform::Identity };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FRotator ActorDeltaRotation { FRotator::ZeroRotator };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FTransform ComponentLocalTransform { FTransform::Identity };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FQuat MeshComponentRelativeRotation { FQuat::Identity };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FRotator ControllerRotation { FRotator::ZeroRotator };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	FVector Acceleration { FVector::ZeroVector };

	/// Used only when displaying the debug trajectory. This is not fenced by
	/// WITH_EDITORDATA_ONLY checks so that the value can be set in other code
	/// without being conditional, but it will only be USED when debugging in
	/// the editor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trajectory")
	bool bUseAsMarker { false };

	FGMCE_MovementSample(const FTransform& TransformWorldSpace, const FVector& LinearVelocity)
	{
		AccumulatedSeconds = 0.f;
		WorldTransform = TransformWorldSpace;
		WorldLinearVelocity = LinearVelocity;

		RelativeTransform = WorldTransform * WorldTransform.Inverse();
		RelativeLinearVelocity = WorldTransform.Inverse().TransformVectorNoScale(WorldLinearVelocity);
	}

	FGMCE_MovementSample() {};

	FRotator GetRotationVelocityFrom(const FGMCE_MovementSample& OtherSample, const EGMCE_TrajectoryRotationType& RotationType) const
	{
		if ( Equals(OtherSample, true) ) return FRotator::ZeroRotator;

		const float TimeRatio = 1.f / ( AccumulatedSeconds - OtherSample.AccumulatedSeconds );

		FRotator Delta;
		switch (RotationType)
		{
		case EGMCE_TrajectoryRotationType::Actor:
			Delta = (ActorWorldRotation - OtherSample.ActorWorldRotation).GetNormalized();
			break;
		case EGMCE_TrajectoryRotationType::Component:
			Delta = (WorldTransform.GetRotation().Rotator() - OtherSample.WorldTransform.GetRotation().Rotator()).GetNormalized();
			break;
		case EGMCE_TrajectoryRotationType::Controller:
			Delta = (ControllerRotation - OtherSample.ControllerRotation).GetNormalized();
			break;
		case EGMCE_TrajectoryRotationType::MeshOffset:
			Delta = (MeshComponentRelativeRotation.Rotator() - OtherSample.MeshComponentRelativeRotation.Rotator()).GetNormalized();
			break;
		case EGMCE_TrajectoryRotationType::Travel:
			Delta = FQuat::FindBetween(OtherSample.WorldLinearVelocity, WorldLinearVelocity).Rotator().GetNormalized();
			break;
		}

		if (Delta.IsNearlyZero()) return FRotator::ZeroRotator;
		
		// World points don't change.
		return FRotator(Delta.Pitch * TimeRatio, Delta.Yaw * TimeRatio, Delta.Roll * TimeRatio);		
	}

	FVector GetAccelerationFrom(const FGMCE_MovementSample& OtherSample) const
	{
		if ( Equals(OtherSample, true) ) return FVector::ZeroVector;

		const float TimeRatio = 1.f / ( AccumulatedSeconds - OtherSample.AccumulatedSeconds );

		const FVector Travel = WorldLinearVelocity - OtherSample.WorldLinearVelocity;
		return Travel / TimeRatio;
	}

	bool IsZeroSample() const
	{
		return RelativeLinearVelocity.IsNearlyZero() &&
			RelativeTransform.GetTranslation().IsNearlyZero() &&
			RelativeTransform.GetRotation().IsIdentity();		
	}

	void PrependRelativeOffset(const FTransform& DeltaTransform, float DeltaSeconds)
	{
		AccumulatedSeconds += DeltaSeconds;
		RelativeTransform *= DeltaTransform;
		RelativeLinearVelocity = DeltaTransform.TransformVectorNoScale(RelativeLinearVelocity);		
	}

	float DistanceFrom(const FGMCE_MovementSample& OtherSample) const
	{
		return FVector::Distance(WorldTransform.GetLocation(), OtherSample.WorldTransform.GetLocation());
	}

	FGMCE_MovementSample Lerp(const FGMCE_MovementSample& Other, float Alpha) const
	{
		FGMCE_MovementSample NewSample;

		FQuat NewRotation = FQuat::FastLerp(WorldTransform.GetRotation(), Other.WorldTransform.GetRotation(), Alpha);
		FVector NewPosition = FMath::Lerp(WorldTransform.GetLocation(), Other.WorldTransform.GetLocation(), Alpha);
		NewSample.WorldTransform = FTransform(NewRotation, NewPosition);

		NewRotation = FQuat::FastLerp(RelativeTransform.GetRotation(), Other.RelativeTransform.GetRotation(), Alpha);
		NewPosition = FMath::Lerp(RelativeTransform.GetLocation(), Other.RelativeTransform.GetLocation(), Alpha);
		NewSample.RelativeTransform = FTransform(NewRotation, NewPosition);

		NewRotation = FQuat::FastLerp(ActorWorldTransform.GetRotation(), Other.ActorWorldTransform.GetRotation(), Alpha);
		NewPosition = FMath::Lerp(ActorWorldTransform.GetLocation(), Other.ActorWorldTransform.GetLocation(), Alpha);
		NewSample.ActorWorldTransform = FTransform(NewRotation, NewPosition);
		
		NewSample.ActorDeltaRotation = FMath::Lerp(ActorDeltaRotation, Other.ActorDeltaRotation, Alpha);
		NewSample.ControllerRotation = FMath::Lerp(ControllerRotation, Other.ControllerRotation, Alpha);
		NewSample.MeshComponentRelativeRotation = FQuat::FastLerp(MeshComponentRelativeRotation, Other.MeshComponentRelativeRotation, Alpha);
		
		NewSample.WorldLinearVelocity = FMath::Lerp(WorldLinearVelocity, Other.WorldLinearVelocity, Alpha);
		NewSample.RelativeLinearVelocity = FMath::Lerp(RelativeLinearVelocity, Other.RelativeLinearVelocity, Alpha);

		NewSample.Acceleration = FMath::Lerp(Acceleration, Other.Acceleration, Alpha);
		
		NewSample.AccumulatedSeconds = FMath::Lerp(AccumulatedSeconds, Other.AccumulatedSeconds, Alpha);

		return NewSample;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("[%f] %s facing %s, accel %s, controller %s"), AccumulatedSeconds,
			*WorldLinearVelocity.ToCompactString(), *WorldTransform.GetRotation().Rotator().ToCompactString(), *Acceleration.ToCompactString(), *ControllerRotation.ToCompactString());
	}

	void Reset()
	{
		AccumulatedSeconds = 0.f;
		WorldTransform = RelativeTransform = FTransform::Identity;
		WorldLinearVelocity = RelativeLinearVelocity = FVector::ZeroVector;
		RelativeTransform = FTransform::Identity;
		RelativeLinearVelocity = FVector::ZeroVector;
		ActorWorldRotation = FRotator::ZeroRotator;
		ActorDeltaRotation = FRotator::ZeroRotator;
		ControllerRotation = FRotator::ZeroRotator;
		ActorWorldTransform = FTransform::Identity;
		MeshComponentRelativeRotation = FQuat::Identity;
	}

	void operator =(const FGMCE_MovementSample& Other)
	{
		AccumulatedSeconds = Other.AccumulatedSeconds;
		RelativeTransform = Other.RelativeTransform;
		RelativeLinearVelocity = Other.RelativeLinearVelocity;
		WorldTransform = Other.WorldTransform;
		WorldLinearVelocity = Other.WorldLinearVelocity;
		ActorWorldRotation = Other.ActorWorldRotation;
		ActorDeltaRotation = Other.ActorDeltaRotation;
		ControllerRotation = Other.ControllerRotation;
	}

	bool Equals(const FGMCE_MovementSample& Other, const bool bIgnoreTime = false) const
	{
		return (bIgnoreTime || AccumulatedSeconds == Other.AccumulatedSeconds) &&
				WorldTransform.Equals(Other.WorldTransform) &&
				WorldLinearVelocity == Other.WorldLinearVelocity &&
				RelativeTransform.Equals(Other.RelativeTransform) &&
				RelativeLinearVelocity == Other.RelativeLinearVelocity &&
				ControllerRotation.Equals(Other.ControllerRotation);
	}

	bool operator ==(const FGMCE_MovementSample& Other) const
	{
		return Equals(Other);
	}

	void DrawDebug(const UWorld* World, const FTransform& FromOrigin = FTransform::Identity, const FColor& Color = FColor::Purple, float LifeTime = -1.f) const;

	// ReSharper disable once CppNonExplicitConversionOperator
	operator FTransformTrajectorySample() const
	{
		FTransformTrajectorySample Result;

		Result.TimeInSeconds = AccumulatedSeconds;
		Result.Position = WorldTransform.GetTranslation();  
		Result.Facing = WorldTransform.GetRotation();

		return Result;
	}
};

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FGMCE_MovementSampleCollection
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trajectory")
	TArray<FGMCE_MovementSample> Samples;

	void DrawDebug(const UWorld* World, const FTransform& FromOrigin, const FColor& PastColor = FColor::Blue,
		const FColor& PresentColor = FColor::Purple, const FColor& FutureColor = FColor::Red, int NumPastSamples = -1, float LifeTime = -1.f) const;

	FGMCE_MovementSample GetSampleAtTime(const float Time, bool bExtrapolate) const
	{
		const int32 Num = Samples.Num();
		if (Num > 1)
		{
			if (Time < Samples[0].AccumulatedSeconds) return Samples[0];
			if (Time > Samples.Last().AccumulatedSeconds) return Samples.Last();
			
			const int32 LowerBoundIdx = Algo::LowerBound(Samples, Time, [](const FTransformTrajectorySample& TrajectorySample, float Value)
				{
					return Value > TrajectorySample.TimeInSeconds;
				});

			const int32 NextIdx = FMath::Clamp(LowerBoundIdx, 1, Samples.Num() - 1);
			const int32 PrevIdx = NextIdx - 1;

			const float Denominator = Samples[NextIdx].AccumulatedSeconds - Samples[PrevIdx].AccumulatedSeconds;
			if (!FMath::IsNearlyZero(Denominator))
			{
				const float Numerator = Time - Samples[PrevIdx].AccumulatedSeconds;
				const float LerpValue = bExtrapolate ? Numerator / Denominator : FMath::Clamp(Numerator / Denominator, 0.f, 1.f);
				return Samples[PrevIdx].Lerp(Samples[NextIdx], LerpValue);
			}

			return Samples[PrevIdx];
		}

		if (Num > 0)
		{
			return Samples[0];
		}

		return FGMCE_MovementSample();		
	}

	bool HasMovementInRange(float MinTime, float MaxTime) const
	{
		for (const auto& Sample : Samples)
		{
			if (Sample.AccumulatedSeconds >= MinTime && Sample.AccumulatedSeconds <= MaxTime)
			{
				if (!Sample.WorldLinearVelocity.IsNearlyZero()) return true;
			}
		}

		return false;
	}

	// ReSharper disable once CppNonExplicitConversionOperator
	operator FTransformTrajectory() const
	{
		FTransformTrajectory Result;
		Result.Samples.Reserve(Samples.Num());

		for (const FGMCE_MovementSample& Sample : Samples)
		{
			Result.Samples.Emplace(static_cast<FTransformTrajectorySample>(Sample));
		}

		return Result;
	}
};