// Copyright 2024 Rooibot Games, LLC


#include "GMCE_MovementAnimInstance.h"

#include "GMCExtendedAnimationLog.h"
#include "Animation/AnimCurveCompressionCodec_UniformIndexable.h"
#include "Kismet/KismetMathLibrary.h"

void UGMCE_MovementAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateLocomotionValues(true);

	if (bTurnInPlace) 
	{
		const float YawAbsolute = FMath::Abs(ComponentYawRemaining);
		ComponentYawTimeRemaining = MovementComponent->GetTurnInPlaceDuration();
		if (!bWasLastTurnInPlace || ComponentYawScale == 0.f)
		{
			ComponentTargetYaw = ComponentYawRemaining;
			if (YawAbsolute <= 90.f)
			{
				ComponentYawScale = YawAbsolute / 90.f;
			}
			else
			{
				ComponentYawScale = YawAbsolute / 180.f;
			}
		}
	}
	else
	{
		ComponentYawTimeRemaining = 0.f;
		ComponentYawScale = 1.f;
		ComponentTargetYaw = 0.f;
	}
	bWasLastTurnInPlace = bTurnInPlace;
}

void UGMCE_MovementAnimInstance::UpdateLocomotionValues(bool bUseCurrentValues)
{
	LocomotionQuadrant = CalculateLocomotionQuadrant(LocomotionQuadrant, LocomotionAngle);

	if (LocomotionAnimationMode == EGMCE_LocomotionAnimationMode::NonStrafing || !bIsMoving)
	{
		// No strafing (or standing idle), no orientation!
		LocomotionCompass = EGMCE_LocomotionCompass::Front_0;
		OrientationCompass = EGMCE_LocomotionCompass::Front_0;
		OrientationAngle = 0.f;
	}
	else
	{
		if (LocomotionAnimationMode == EGMCE_LocomotionAnimationMode::Strafing4Way)
		{
			LocomotionCompass = CalculateLocomotionCompass4Way(LocomotionCompass, LocomotionAngle, bUseCurrentValues, 5.f,
														   LocomotionAnimationMode);
			OrientationCompass = CalculateLocomotionCompass4Way(OrientationCompass, OrientationAngle, bUseCurrentValues, 5.f,
															LocomotionAnimationMode);
		}
		else
		{
			LocomotionCompass = CalculateLocomotionCompass8Way(LocomotionCompass, LocomotionAngle, bUseCurrentValues, 10.f,
														   LocomotionAnimationMode);
			OrientationCompass = CalculateLocomotionCompass8Way(OrientationCompass, OrientationAngle, bUseCurrentValues, 10.f,
															LocomotionAnimationMode);
		}
		OrientationAngle = GetOrientationAngleForCompass(LocomotionAngle, LocomotionCompass);
	}

	StrideWarpRatio = 1.f;
	if (AnimationSpeedCurve != NAME_None)
	{
		if (const float SpeedValue = GetCurveValue(AnimationSpeedCurve); SpeedValue > UE_KINDA_SMALL_NUMBER)
		{
			StrideWarpRatio = GroundSpeed / SpeedValue;
		}
	}
}

EGMCE_LocomotionQuadrant UGMCE_MovementAnimInstance::CalculateLocomotionQuadrant(const EGMCE_LocomotionQuadrant& CurrentQuadrant,
                                                                            const float InputAngle)
{
	float Angle = InputAngle;
	if (FMath::Abs(Angle) >= 180.f)
	{
		Angle = UKismetMathLibrary::NormalizeAxis(Angle);
	}

	if (Angle >= 0.f && Angle <= 90.f) { return EGMCE_LocomotionQuadrant::FrontRight; }
	if (Angle >= 90.f && Angle <= 180.f) { return EGMCE_LocomotionQuadrant::BackRight; }
	if (Angle <= 0.f && Angle >= -90.f) { return EGMCE_LocomotionQuadrant::FrontLeft; }
	if (Angle <= -90.f && Angle >= -180.f) { return EGMCE_LocomotionQuadrant::BackLeft; }

	// We should never get here, but let's just have a sanity check anyway.
	return EGMCE_LocomotionQuadrant::FrontLeft;
}

EGMCE_LocomotionCompass UGMCE_MovementAnimInstance::CalculateLocomotionCompass8Way(const EGMCE_LocomotionCompass& CurrentCompass,
                                                                          const float InputAngle,
                                                                          const bool bUseCurrent,
                                                                          const float SwitchBuffer,
                                                                          const EGMCE_LocomotionAnimationMode AnimationMode)
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
			return EGMCE_LocomotionCompass::Back_180;
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
		return EGMCE_LocomotionCompass::Back_180;
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
				return static_cast<EGMCE_LocomotionCompass>(Mid);
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
	return EGMCE_LocomotionCompass::Front_0;
}

EGMCE_LocomotionCompass UGMCE_MovementAnimInstance::CalculateLocomotionCompass4Way(const EGMCE_LocomotionCompass& CurrentCompass,
                                                                          const float InputAngle,
                                                                          const bool bUseCurrent,
                                                                          const float SwitchBuffer,
                                                                          const EGMCE_LocomotionAnimationMode AnimationMode)
const
{
	/** Normalize the angle to be within [-180, 180] */
	const float Angle = (FMath::Abs(InputAngle) > 180.0f)
		                    ? FRotator::NormalizeAxis(InputAngle)
		                    : InputAngle;

	/** Ensure the buffer acts only as a positive value. */
	const float Buffer = FMath::Abs(SwitchBuffer);

	/** Define the animation ranges. We double them up so that we only match
	 *  values for indexes 0, 2, 4, and 6 -- the enumeration values which correspond
	 *  to the four cardinal compass points.
	 */
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
			return EGMCE_LocomotionCompass::Back_180;
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
		return EGMCE_LocomotionCompass::Back_180;
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
				return static_cast<EGMCE_LocomotionCompass>(Mid);
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
	return EGMCE_LocomotionCompass::Front_0;
}

float UGMCE_MovementAnimInstance::GetOrientationAngleForCompass(const float InputAngle,
                                                                const EGMCE_LocomotionCompass CurrentCompass
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
	
	return Result;
}

float UGMCE_MovementAnimInstance::GetAngleForCompass(const EGMCE_LocomotionCompass& CurrentCompass)
{
	switch (CurrentCompass)
	{
	case EGMCE_LocomotionCompass::Back_180:
		return 180.f;
	case EGMCE_LocomotionCompass::Left_135:
		return -135.f;
	case EGMCE_LocomotionCompass::Left_90:
		return -90.f;
	case EGMCE_LocomotionCompass::Left_45:
		return -45.f;
	case EGMCE_LocomotionCompass::Front_0:
		return 0.f;
	case EGMCE_LocomotionCompass::Right_45:
		return 45.f;
	case EGMCE_LocomotionCompass::Right_90:
		return 90.f;
	case EGMCE_LocomotionCompass::Right_135:
		return 135.f;
	}

	return 0.f;
}

float UGMCE_MovementAnimInstance::InitializeCurveTracker(FGMCE_AnimationCurveTracker& Tracker, UAnimSequence* Sequence,
	FName CurveName, float StartTime, float PlayRate, float Scale)
{
	Tracker.CurveName = CurveName;
	Tracker.SourceSequence = Sequence;
	Tracker.AccumulatedTime = StartTime;
	Tracker.PlayRate = PlayRate;
	Tracker.Scale = Scale;

	return GetCurveTrackerValue(Tracker);
}

float UGMCE_MovementAnimInstance::UpdateCurveTracker(FGMCE_AnimationCurveTracker& Tracker, float DeltaTime)
{
	Tracker.AccumulatedTime = FMath::Clamp(Tracker.AccumulatedTime + DeltaTime, 0.f, Tracker.SourceSequence->GetPlayLength() / Tracker.PlayRate);
	return GetCurveTrackerValue(Tracker);
}

float UGMCE_MovementAnimInstance::GetCurveTrackerValue(FGMCE_AnimationCurveTracker& Tracker)
{
	// Exit early if we have no curve data.
	if (!IsValid(Tracker.SourceSequence)) return 0.f;
	if (!Tracker.SourceSequence->HasCurveData(Tracker.CurveName, false)) return 0.f;

	const FAnimCurveBufferAccess CurveBufferAccess(Tracker.SourceSequence, Tracker.CurveName);
	if (!CurveBufferAccess.IsValid()) return 0.f;

	// Shamelessly took this idea for how to narrow in on a key and determine the curve value from
	// Ryan Muoio's excellent Animation Matching Suite.
	const int32 KeyCount = CurveBufferAccess.GetNumSamples();
	if (KeyCount < 2)
	{
		return 0.f;
	}
		
	int32 First = 1;
	const int32 Last = KeyCount - 1;
	int32 Count = Last - First;

	while (Count > 0)
	{
		const int32 Step = Count / 2;
		const int32 Middle = First + Step;

		if (Tracker.AccumulatedTime * Tracker.PlayRate > CurveBufferAccess.GetTime(Middle))
		{
			First = Middle + 1;
			Count -= Step + 1;
		}
		else
		{
			Count = Step;
		}
	}

	const float KeyATime = CurveBufferAccess.GetTime(First - 1);
	const float KeyBTime = CurveBufferAccess.GetTime(First);
	const float Diff = KeyBTime - KeyATime;
	const float Alpha = FMath::Clamp(!FMath::IsNearlyZero(Diff) ? (((Tracker.AccumulatedTime * Tracker.PlayRate) - KeyATime) / Diff) : 0.0f, 0.f, 1.f);

	const float KeyAValue = CurveBufferAccess.GetValue(First - 1);
	const float KeyBValue = CurveBufferAccess.GetValue(First);
	
	const float Result = FMath::Lerp(KeyAValue * Tracker.Scale, KeyBValue * Tracker.Scale, Alpha);
	
	return Result;
}

float UGMCE_MovementAnimInstance::CalculateScaledTurnInPlacePlayRate()
{
	float Result = 1.f;
	if (TurnInPlaceTracker.SourceSequence != nullptr && !FMath::IsNearlyZero(MovementComponent->GetTurnInPlaceDuration()))
	{
		Result = TurnInPlaceTracker.SourceSequence->GetPlayLength() / MovementComponent->GetTurnInPlaceDuration();
	}
	TurnInPlaceTracker.PlayRate = Result;

	return Result;
}

void UGMCE_MovementAnimInstance::InitializeTurnInPlaceTracker(UAnimSequence* Sequence, FName CurveName, float StartTime, float PlayRate)
{
	const float CurveValue = InitializeCurveTracker(TurnInPlaceTracker, Sequence, CurveName, StartTime, PlayRate, ComponentYawScale);

	switch (TurnInPlaceCurveType)
	{
	case EGMCE_TurnInPlaceCurveType::RemainingTurnYaw:
		TurnInPlaceYawCompleted = ComponentTargetYaw - CurveValue;
		break;
	case EGMCE_TurnInPlaceCurveType::CompletedTurnYaw:
		TurnInPlaceYawCompleted = CurveValue;
		break;
	}

	if (PlayRate == 0.f)
	{
		// Calculate play rate.
		CalculateScaledTurnInPlacePlayRate();
	}
}

void UGMCE_MovementAnimInstance::UpdateTurnInPlaceTracker(float DeltaTime)
{
	float CurveValue = 0.f;
	
	if (TurnInPlaceTracker.AccumulatedTime + DeltaTime > TurnInPlaceTracker.SourceSequence->GetPlayLength() / TurnInPlaceTracker.PlayRate)
	{
		// We've run past the end of the animation; ensure that our movement component knows we're done.
		TurnInPlaceYawCompleted = ComponentTargetYaw;
	}
	else {
		CurveValue = UpdateCurveTracker(TurnInPlaceTracker, DeltaTime);

		switch (TurnInPlaceCurveType)
		{
		case EGMCE_TurnInPlaceCurveType::RemainingTurnYaw:
			TurnInPlaceYawCompleted = ComponentTargetYaw - CurveValue;
			break;
		case EGMCE_TurnInPlaceCurveType::CompletedTurnYaw:
			TurnInPlaceYawCompleted = CurveValue;
			break;
		}
	}
}

float UGMCE_MovementAnimInstance::GetTrackedCurve(const FName CurveName) const
{
	float Value = 0.f;
	GetTrackedCurve(CurveName, Value);
	return Value;
}

bool UGMCE_MovementAnimInstance::GetTrackedCurve(const FName CurveName, float& OutValue) const
{
	if (CurveName == FName(TEXT("TurnInPlace")))
	{
		OutValue = TurnInPlaceYawCompleted;
		return true;
	}

	OutValue = 0.f;
	return false;
}
