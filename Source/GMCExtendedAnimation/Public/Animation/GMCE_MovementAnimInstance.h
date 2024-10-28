﻿// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "GMCE_BaseAnimInstance.h"
#include "GMCE_TrackedCurveProvider.h"
#include "Animation/AnimInstance.h"
#include "GMCE_MovementAnimInstance.generated.h"

/// Quadrants of movement, clockwise from the front right.
UENUM(BlueprintType)
enum class EGMCE_LocomotionQuadrant : uint8
{
	FrontRight UMETA(ToolTip="Character locomotion is in the range [0, 90]"),
	BackRight UMETA(ToolTip="Character locomotion is in the range [90, 180]"),
	BackLeft UMETA(ToolTip="Character locomotion is in the range [-90, -180]"),
	FrontLeft UMETA(ToolTip="Character locomotion is in the range [0, -90]")
	
};

/// Locomotion compass points, clockwise from forward.
UENUM(BlueprintType)
enum class EGMCE_LocomotionCompass : uint8
{
	Back_180	UMETA(DisplayName="Backward (180°)"),
	Left_135    UMETA(DisplayName="Leftward (135°)"),
	Left_90     UMETA(DisplayName="Leftward (90°)"),
	Left_45	    UMETA(DisplayName="Leftward (45°)"),
	Front_0     UMETA(DisplayName="Forward (0°)"),
	Right_45    UMETA(DisplayName="Rightward (45°)"),
	Right_90    UMETA(DisplayName="Rightward (90°)"),
	Right_135   UMETA(DisplayName="Rightward (135°)")
};

UENUM(BlueprintType)
enum class EGMCE_LocomotionAnimationMode : uint8
{ 
	NonStrafing		UMETA(DisplayName="Non-Strafing", ToolTip="Not strafing at all; the character will always face the direction they're moving and only uses a forward animation.."),
	Strafing4Way	UMETA(DisplayName="4-Way Strafing", ToolTip="Strafing using four directional animations"),
	Strafing8Way	UMETA(DisplayName="8-Way Strafing", ToolTip="Strafing using eight directional animations")
};

UENUM(BlueprintType)
enum class EGMCE_TurnInPlaceCurveType : uint8
{
	RemainingTurnYaw,
	CompletedTurnYaw
};

USTRUCT(BlueprintType)
struct GMCEXTENDEDANIMATION_API FGMCE_AnimationCurveTracker
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category="Curve")
	FName CurveName { NAME_None };

	UPROPERTY(BlueprintReadOnly, Category="Curve")
	UAnimSequence* SourceSequence { nullptr };

	UPROPERTY(BlueprintReadOnly, Category="Curve")
	float AccumulatedTime { 0.f };

	UPROPERTY(BlueprintReadOnly, Category="Curve")
	float PlayRate { 1.f };

	UPROPERTY(BlueprintReadOnly, Category="Curve")
	float Scale { 1.f };
};


/**
 * This is an extension of the Base Animation Instance which not only provides the calculated values, but also
 * a number of useful functions to turn the calculated values into actual animation.
 */
UCLASS(meta=(DisplayName="GMC Extended Movement Animation Blueprint"))
class GMCEXTENDEDANIMATION_API UGMCE_MovementAnimInstance : public UGMCE_BaseAnimInstance, public IGMCE_TrackedCurveProvider
{
	GENERATED_BODY()

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orientation")
	EGMCE_LocomotionAnimationMode LocomotionAnimationMode { EGMCE_LocomotionAnimationMode::NonStrafing };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	EGMCE_LocomotionQuadrant LocomotionQuadrant { EGMCE_LocomotionQuadrant::FrontRight };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	EGMCE_LocomotionCompass LocomotionCompass { EGMCE_LocomotionCompass::Front_0 };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	EGMCE_LocomotionCompass OrientationCompass { EGMCE_LocomotionCompass::Front_0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion")
	FName AnimationSpeedCurve { FName(TEXT("Speed")) };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Locomotion")
	float StrideWarpRatio { 1.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	float ComponentYawTimeRemaining { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	float ComponentYawScale { 1.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	float ComponentTargetYaw { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	float TurnInPlaceYawCompleted { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	FGMCE_AnimationCurveTracker TurnInPlaceTracker;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Turn-in-Place")
	EGMCE_TurnInPlaceCurveType TurnInPlaceCurveType { EGMCE_TurnInPlaceCurveType::RemainingTurnYaw };

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Turn-in-Place")
	FName TurnInPlaceTrack { FName(TEXT("RemainingTurnYaw")) };
	
	bool bWasLastTurnInPlace { false };

	virtual void UpdateLocomotionValues(bool bUseCurrentValues = true);

	static EGMCE_LocomotionQuadrant CalculateLocomotionQuadrant(const EGMCE_LocomotionQuadrant& CurrentQuadrant, const float InputAngle);
	EGMCE_LocomotionCompass CalculateLocomotionCompass8Way(const EGMCE_LocomotionCompass& CurrentCompass, float InputAngle,
	                                              bool bUseCurrent, float SwitchBuffer,
	                                              EGMCE_LocomotionAnimationMode AnimationMode) const;
	EGMCE_LocomotionCompass CalculateLocomotionCompass4Way(const EGMCE_LocomotionCompass& CurrentCompass, float InputAngle,
												  bool bUseCurrent, float SwitchBuffer,
												  EGMCE_LocomotionAnimationMode AnimationMode) const;


	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement|Orientation", meta=(BlueprintThreadSafe))
	static float GetOrientationAngleForCompass(const float InputAngle, const EGMCE_LocomotionCompass CurrentCompass);

	static float GetAngleForCompass(const EGMCE_LocomotionCompass& CurrentCompass);

	UFUNCTION(BlueprintCallable, Category="Animation", meta=(BlueprintThreadSafe))
	static float InitializeCurveTracker(UPARAM(ref) FGMCE_AnimationCurveTracker& Tracker, UAnimSequence* Sequence, FName CurveName, float StartTime = 0.f, float PlayRate = 1.f, float Scale = 1.f);

	UFUNCTION(BlueprintCallable, Category="Animation", meta=(BlueprintThreadSafe))
	static float UpdateCurveTracker(UPARAM(ref) FGMCE_AnimationCurveTracker& Tracker, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category="Animation", meta=(BlueprintThreadSafe))
	static float GetCurveTrackerValue(UPARAM(ref) FGMCE_AnimationCurveTracker& Tracker);

	UFUNCTION(BlueprintCallable, Category="Animation", meta=(BlueprintThreadSafe))
	float CalculateScaledTurnInPlacePlayRate();
	
	UFUNCTION(BlueprintCallable, Category="Animation", meta=(BlueprintThreadSafe))
	void InitializeTurnInPlaceTracker(UAnimSequence* Sequence, FName CurveName, float StartTime = 0.f, float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable, Category="Animation", meta=(BlueprintThreadSafe))
	void UpdateTurnInPlaceTracker(float DeltaTime);

	virtual float GetTrackedCurve(const FName CurveName) const override;
	virtual bool GetTrackedCurve(const FName CurveName, float& OutValue) const override;
	
};
