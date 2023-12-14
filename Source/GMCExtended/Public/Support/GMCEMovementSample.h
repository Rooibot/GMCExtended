/* ROOIBOT CORE FRAMEWORK
 * Copyright 2023, Rooibot Games, LLC. - All rights reserved.
 *
 * The URGTrajectoryMovementComponent and support files have been made
 * available for the use of other licensees of GRIMTEC's Unreal Engine 5
 * plugin "General Movement Component v2". They may be redistributed to 
 * other GMCv2 licensees, provided this notice remains intact.
 *
 * Questions can be addressed to Rachel Blackman at either 
 * rachel.blackman@rooibot.com or as "Packetdancer" on Discord.
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/MotionTrajectoryTypes.h"
#include "GMCEMovementSample.generated.h"

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FGMCE_MovementSample
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AccumulatedSeconds { 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform RelativeTransform { FTransform::Identity };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector RelativeLinearVelocity { 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform WorldTransform { FTransform::Identity };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector WorldLinearVelocity { 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FRotator ActorWorldRotation { FRotator::ZeroRotator };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FRotator ActorDeltaRotation { FRotator::ZeroRotator };

	/// Used only when displaying the debug trajectory. This is not fenced by
	/// WITH_EDITORDATA_ONLY checks so that the value can be set in other code
	/// without being conditional, but it will only be USED when debugging in
	/// the editor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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

	FRotator GetRotationVelocityFrom(const FGMCE_MovementSample& OtherSample) const
	{
		if ( Equals(OtherSample, true) ) return FRotator::ZeroRotator;
	
		const float TimeRatio = 1.f / ( AccumulatedSeconds - OtherSample.AccumulatedSeconds );
	
		// World points don't change.
		const FRotator Delta = (ActorWorldRotation - OtherSample.ActorWorldRotation).GetNormalized();
		return FRotator(Delta.Pitch * TimeRatio, Delta.Yaw * TimeRatio, Delta.Roll * TimeRatio);		
	}

	FVector GetAccelerationFrom(const FGMCE_MovementSample& OtherSample) const
	{
		if ( Equals(OtherSample, true) ) return FVector::ZeroVector;

		const float TimeRatio = 1.f / ( AccumulatedSeconds - OtherSample.AccumulatedSeconds );

		const FVector Travel = WorldTransform.GetLocation() - OtherSample.WorldTransform.GetLocation();
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

	FString ToString() const
	{
		return FString::Printf(TEXT("[%f] %s %s %s - { %s %s %s }"), AccumulatedSeconds,
			*RelativeTransform.GetLocation().ToCompactString(),
			*RelativeTransform.GetRotation().Rotator().ToCompactString(),
			*RelativeLinearVelocity.ToCompactString(),
			*WorldTransform.GetLocation().ToCompactString(),
			*WorldTransform.GetRotation().Rotator().ToCompactString(),
			*WorldLinearVelocity.ToCompactString());		
	}

	void Reset()
	{
		AccumulatedSeconds = 0.f;
		WorldTransform = RelativeTransform = FTransform::Identity;
		WorldLinearVelocity = RelativeLinearVelocity = FVector::ZeroVector;
		ActorWorldRotation = FRotator::ZeroRotator;
		ActorDeltaRotation = FRotator::ZeroRotator;
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
	}

	bool Equals(const FGMCE_MovementSample& Other, const bool bIgnoreTime = false) const
	{
		return (bIgnoreTime || AccumulatedSeconds == Other.AccumulatedSeconds) &&
				WorldTransform.Equals(Other.WorldTransform) &&
				WorldLinearVelocity == Other.WorldLinearVelocity &&
				RelativeTransform.Equals(Other.RelativeTransform) &&
				RelativeLinearVelocity == Other.RelativeLinearVelocity;
	}

	bool operator ==(const FGMCE_MovementSample& Other) const
	{
		return Equals(Other);
	}

	void DrawDebug(const UWorld* World, const FTransform& FromOrigin = FTransform::Identity, const FColor& Color = FColor::Purple) const;
	
	explicit operator FTrajectorySample() const
	{
		FTrajectorySample Result;
		
		Result.LinearVelocity = RelativeLinearVelocity;
		Result.AccumulatedSeconds = AccumulatedSeconds;
		Result.Transform = RelativeTransform;

		return Result;
	}
	
};

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FGMCE_MovementSampleCollection
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FGMCE_MovementSample> Samples;

	void DrawDebug(const UWorld* World, const FTransform& FromOrigin, const FColor& PastColor = FColor::Blue,
		const FColor& FutureColor = FColor::Red) const;
	
	explicit operator FTrajectorySampleRange() const
	{
		FTrajectorySampleRange Result;
		Result.Samples.Reserve(Samples.Num());

		for (const FGMCE_MovementSample& Sample : Samples)
		{
			// Implicit conversion gives us an FTrajectorySample
			Result.Samples.Emplace(static_cast<FTrajectorySample>(Sample));
		}

		return Result;
	}
};