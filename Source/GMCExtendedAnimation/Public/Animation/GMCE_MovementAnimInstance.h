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
	Forward UMETA(ToolTip="Character locomotion is close to 0"),
	ForwardRight UMETA(ToolTip="Character locomotion is close to 45"),
	Right UMETA(ToolTip="Character locomotion is close to 90"),
	BackwardRight UMETA(ToolTip="Character locomotion is close to 135"),
	Backward UMETA(ToolTip="Character locomotion is close to 180 (or -180)"),
	BackwardLeft UMETA(ToolTip="Character locomotion is close to -135"),
	Left UMETA(ToolTip="Character locomotion is close to -90"),
	ForwardLeft UMETA(ToolTip="Character locomotion is close to -45")
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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	ELocomotionQuadrant LocomotionQuadrant { ELocomotionQuadrant::FrontRight };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	ELocomotionCompass LocomotionCompass { ELocomotionCompass::Forward };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	ELocomotionCompass OrientationCompass { ELocomotionCompass::Forward };
	
};
