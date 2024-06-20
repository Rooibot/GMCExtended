// Copyright 2024 Rooibot Games, LLC


#include "GMCE_MovementAnimInstance.h"

#include "GMCExtendedAnimationLog.h"
#include "Kismet/KismetMathLibrary.h"

void UGMCE_MovementAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateLocomotionValues(true);
}

void UGMCE_MovementAnimInstance::UpdateLocomotionValues(bool bUseCurrentValues)
{
	LocomotionQuadrant = CalculateLocomotionQuadrant(LocomotionQuadrant, LocomotionAngle);

	if (LocomotionAnimationMode == ELocomotionAnimationMode::NonStrafing || !bIsMoving)
	{
		// No strafing (or standing idle), no orientation!
		LocomotionCompass = ELocomotionCompass::Front_0;
		OrientationCompass = ELocomotionCompass::Front_0;
		OrientationAngle = 0.f;
	}
	else
	{
		if (LocomotionAnimationMode == ELocomotionAnimationMode::Strafing4Way)
		{
			LocomotionCompass = CalculateLocomotionCompass4Way(LocomotionCompass, LocomotionAngle, bUseCurrentValues, 5.f,
														   LocomotionAnimationMode);
			OrientationCompass = CalculateLocomotionCompass4Way(OrientationCompass, OrientationAngle, bUseCurrentValues, 5.f,
															LocomotionAnimationMode);
8		}
		else
		{
			LocomotionCompass = CalculateLocomotionCompass8Way(LocomotionCompass, LocomotionAngle, bUseCurrentValues, 10.f,
														   LocomotionAnimationMode);
			OrientationCompass = CalculateLocomotionCompass8Way(OrientationCompass, OrientationAngle, bUseCurrentValues, 10.f,
															LocomotionAnimationMode);
		}
		OrientationAngle = GetOrientationAngleForCompass(LocomotionAngle, LocomotionCompass);
	}
}

ELocomotionQuadrant UGMCE_MovementAnimInstance::CalculateLocomotionQuadrant(const ELocomotionQuadrant& CurrentQuadrant,
                                                                            const float InputAngle)
{
	float Angle = InputAngle;
	if (FMath::Abs(Angle) >= 180.f)
	{
		Angle = UKismetMathLibrary::NormalizeAxis(Angle);
	}

	if (Angle >= 0.f && Angle <= 90.f) { return ELocomotionQuadrant::FrontRight; }
	if (Angle >= 90.f && Angle <= 180.f) { return ELocomotionQuadrant::BackRight; }
	if (Angle <= 0.f && Angle >= -90.f) { return ELocomotionQuadrant::FrontLeft; }
	if (Angle <= -90.f && Angle >= -180.f) { return ELocomotionQuadrant::BackLeft; }

	// We should never get here, but let's just have a sanity check anyway.
	return ELocomotionQuadrant::FrontLeft;
}

ELocomotionCompass UGMCE_MovementAnimInstance::CalculateLocomotionCompass8Way(const ELocomotionCompass& CurrentCompass,
                                                                          const float InputAngle,
                                                                          const bool bUseCurrent,
                                                                          const float SwitchBuffer,
                                                                          const ELocomotionAnimationMode AnimationMode)
const
{
	/** Normalize the angle to be within [-180, 180] */
	const float Angle = (FMath::Abs(InputAngle) > 180.0f)
		                    ? FRotator::NormalizeAxis(InputAngle)
		                    : InputAngle;

	/** Ensure the buffer acts only as a positive value. */
	const float Buffer = FMath::Abs(SwitchBuffer);

	/** Define the animation ranges. */
	constexpr float AngleRanges[] = {
		157.5f, -157.5f, // Back_180
		-157.5f, -112.5f, // Left_135
		-112.5f, -67.5f, // Left_090
		-67.5f, -22.5f, // Left_045
		-22.5f, 22.5f, // Front_000
		22.5f, 67.5f, // Right_045
		67.5f, 112.5f, // Right_090
		112.5f, 157.5f // Right_135
	};

	if (bUseCurrent)
	{
		// Special case, since we know if we're bigger than our lowest positive backwards value, we're moving backwards.
		if (FMath::Abs(Angle) >= AngleRanges[0])
		{
			return ELocomotionCompass::Back_180;
		}
		
		const uint8 CompassIndex = static_cast<uint8>(CurrentCompass);
		const float CompassMin = AngleRanges[CompassIndex * 2];
		const float CompassMax = AngleRanges[CompassIndex * 2 + 1];

		if (CompassMin <= CompassMax)
		{
			if (Angle >= CompassMin - Buffer && Angle <= CompassMax + Buffer)
			{
				// We're within the buffer zone, stick with our current value.
				return CurrentCompass;
			}
		}
	}

	// Special case, since we know if we're bigger than our lowest positive backwards value, we're moving backwards.
	if (FMath::Abs(Angle) >= AngleRanges[0] + Buffer)
	{
		return ELocomotionCompass::Back_180;
	}

	/** Perform a binary search. */
	int32 Low = 0;
	int32 High = sizeof(AngleRanges) / sizeof(float) / 2 - 1; // gets the array size minus one
	while (Low <= High)
	{
		int32 Mid = (Low + High) / 2;

		/** Check if LocomotionAngle is within the current range. */
		const float MidMin = AngleRanges[Mid * 2];
		const float MidMax = AngleRanges[Mid * 2 + 1];

		if (MidMin <= MidMax)
		{
			if (Angle >= MidMin && Angle < MidMax)
			{
				return static_cast<ELocomotionCompass>(Mid);
			}
		}

		if (Angle < AngleRanges[Mid * 2])
		{
			High = Mid - 1;
		}
		else
		{
			Low = Mid + 1;
		}
	}

	// Fall through to a default.
	return ELocomotionCompass::Front_0;
}

ELocomotionCompass UGMCE_MovementAnimInstance::CalculateLocomotionCompass4Way(const ELocomotionCompass& CurrentCompass,
                                                                          const float InputAngle,
                                                                          const bool bUseCurrent,
                                                                          const float SwitchBuffer,
                                                                          const ELocomotionAnimationMode AnimationMode)
const
{
	/** Normalize the angle to be within [-180, 180] */
	const float Angle = (FMath::Abs(InputAngle) > 180.0f)
		                    ? FRotator::NormalizeAxis(InputAngle)
		                    : InputAngle;

	/** Ensure the buffer acts only as a positive value. */
	const float Buffer = FMath::Abs(SwitchBuffer);

	/** Define the animation ranges. */
	constexpr float AngleRanges[] = {
		125.f, -125.f, // Backward
		125.f, -125.f, // --
		-145.f, -50.f, // Left
		-145.f, -50.f, // --
		-50.f, 50.f, // Forward
		-50.f, 50.f, // --
		50.f, 125.f, // Right
		50.f, 125.f // --
	};

	if (bUseCurrent)
	{
		// Special case, since we know if we're bigger than our lowest positive backwards value, we're moving backwards.
		if (FMath::Abs(Angle) >= AngleRanges[0])
		{
			return ELocomotionCompass::Back_180;
		}
		
		const uint8 CompassIndex = static_cast<uint8>(CurrentCompass);
		const float CompassMin = AngleRanges[CompassIndex * 2];
		const float CompassMax = AngleRanges[CompassIndex * 2 + 1];

		if (CompassMin <= CompassMax)
		{
			if (Angle >= CompassMin - Buffer && Angle <= CompassMax + Buffer)
			{
				// We're within the buffer zone, stick with our current value.
				return CurrentCompass;
			}
		}
	}

	// Special case, since we know if we're bigger than our lowest positive backwards value, we're moving backwards.
	if (FMath::Abs(Angle) >= AngleRanges[0] + Buffer)
	{
		return ELocomotionCompass::Back_180;
	}

	/** Perform a binary search. */
	int32 Low = 0;
	int32 High = sizeof(AngleRanges) / sizeof(float) / 2 - 1; // gets the array size minus one
	while (Low <= High)
	{
		int32 Mid = (Low + High) / 2;

		/** Check if LocomotionAngle is within the current range. */
		const float MidMin = AngleRanges[Mid * 2];
		const float MidMax = AngleRanges[Mid * 2 + 1];

		if (MidMin <= MidMax)
		{
			if (Angle >= MidMin && Angle < MidMax)
			{
				return static_cast<ELocomotionCompass>(Mid);
			}
		}

		if (Angle < AngleRanges[Mid * 2])
		{
			High = Mid - 1;
		}
		else
		{
			Low = Mid + 1;
		}
	}

	// Fall through to a default.
	return ELocomotionCompass::Front_0;
}

float UGMCE_MovementAnimInstance::GetOrientationAngleForCompass(const float InputAngle,
                                                                const ELocomotionCompass CurrentCompass
                                                                )
{
	float Angle = InputAngle;
	if (FMath::Abs(Angle) > 180.f)
	{
		Angle = UKismetMathLibrary::NormalizeAxis(InputAngle);
	}

	float AnimationAngle = GetAngleForCompass(CurrentCompass);
	if (AnimationAngle == 180.f && Angle < 0.f) AnimationAngle = -180.f;
	
	float Result = Angle - AnimationAngle;
	if (FMath::Abs(Result) > 180.f)
	{
		Result = UKismetMathLibrary::NormalizeAxis(Result);
	}

	if (FMath::Abs(Result) > 90.f)
	{
		Result += 180.f;
		if (Result > 180.f)
		{
			Result -= 360.f;
		}
	}
	
	UE_LOG(LogGMCExAnimation, Warning, TEXT("%f from %f -> %f"), InputAngle, AnimationAngle, Result);
	
	return Result;
}

float UGMCE_MovementAnimInstance::GetAngleForCompass(const ELocomotionCompass& CurrentCompass)
{
	switch (CurrentCompass)
	{
	case ELocomotionCompass::Back_180:
		return 180.f;
	case ELocomotionCompass::Left_135:
		return -135.f;
	case ELocomotionCompass::Left_90:
		return -90.f;
	case ELocomotionCompass::Left_45:
		return -45.f;
	case ELocomotionCompass::Front_0:
		return 0.f;
	case ELocomotionCompass::Right_45:
		return 45.f;
	case ELocomotionCompass::Right_90:
		return 90.f;
	case ELocomotionCompass::Right_135:
		return 135.f;
	}

	return 0.f;
}