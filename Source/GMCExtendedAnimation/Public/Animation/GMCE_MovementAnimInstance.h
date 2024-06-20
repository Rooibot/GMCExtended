// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "GMCE_BaseAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "GMCE_MovementAnimInstance.generated.h"

/// Quadrants of movement, clockwise from the front right.
UENUM(BlueprintType)
enum class ELocomotionQuadrant : uint8
{
	FrontRight UMETA(ToolTip="Character locomotion is in the range [0, 90]"),
	BackRight UMETA(ToolTip="Character locomotion is in the range [90, 180]"),
	BackLeft UMETA(ToolTip="Character locomotion is in the range [-90, -180]"),
	FrontLeft UMETA(ToolTip="Character locomotion is in the range [0, -90]")
	
};

/// Locomotion compass points, clockwise from forward.
UENUM(BlueprintType)
enum class ELocomotionCompass : uint8
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
enum class ELocomotionAnimationMode : uint8
{ 
	NonStrafing		UMETA(DisplayName="Non-Strafing", ToolTip="Not strafing at all; the character will always face the direction they're moving and only uses a forward animation.."),
	Strafing4Way	UMETA(DisplayName="4-Way Strafing", ToolTip="Strafing using four directional animations"),
	Strafing8Way	UMETA(DisplayName="8-Way Strafing", ToolTip="Strafing using eight directional animations")
};

/**
 * This is an extension of the Base Animation Instance which not only provides the calculated values, but also
 * a number of useful functions to turn the calculated values into actual animation.
 */
UCLASS(meta=(DisplayName="GMC Extended Movement Animation Blueprint"))
class GMCEXTENDEDANIMATION_API UGMCE_MovementAnimInstance : public UGMCE_BaseAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Orientation")
	ELocomotionAnimationMode LocomotionAnimationMode { ELocomotionAnimationMode::NonStrafing };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	ELocomotionQuadrant LocomotionQuadrant { ELocomotionQuadrant::FrontRight };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	ELocomotionCompass LocomotionCompass { ELocomotionCompass::Front_0 };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	ELocomotionCompass OrientationCompass { ELocomotionCompass::Front_0 };

	virtual void UpdateLocomotionValues(bool bUseCurrentValues = true);

	static ELocomotionQuadrant CalculateLocomotionQuadrant(const ELocomotionQuadrant& CurrentQuadrant, const float InputAngle);
	ELocomotionCompass CalculateLocomotionCompass8Way(const ELocomotionCompass& CurrentCompass, float InputAngle,
	                                              bool bUseCurrent, float SwitchBuffer,
	                                              ELocomotionAnimationMode AnimationMode) const;
	ELocomotionCompass CalculateLocomotionCompass4Way(const ELocomotionCompass& CurrentCompass, float InputAngle,
												  bool bUseCurrent, float SwitchBuffer,
												  ELocomotionAnimationMode AnimationMode) const;


	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement|Orientation", meta=(BlueprintThreadSafe))
	static float GetOrientationAngleForCompass(const float InputAngle, const ELocomotionCompass CurrentCompass);

	static float GetAngleForCompass(const ELocomotionCompass& CurrentCompass);
	
};
