#include "GMCE_BaseAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Kismet/KismetMathLibrary.h"

void UGMCE_BaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (!IsValid(OwnerPawn) || !IsValid(MovementComponent))
	{
		
#if WITH_EDITOR
		if (!GetWorld()->IsGameWorld())
		{
			// If we're an editor preview, just make sure we have valid data so we don't, y'know, crash.
			if (!OwnerPawn) OwnerPawn = EditorPreviewClass->GetDefaultObject<AGMC_Pawn>();
			MovementComponent = Cast<UGMCE_OrganicMovementCmp>(OwnerPawn->GetMovementComponent());

			if (!MovementComponent)
			{
				MovementComponent = NewObject<UGMCE_OrganicMovementCmp>();
			}

			return;
		}
#endif
		
		if (!OwnerPawn)
		{
			AActor* OwningActor = GetOwningActor();
			OwnerPawn = Cast<AGMC_Pawn>(OwningActor);
		}

		if (OwnerPawn && !MovementComponent)
		{
			MovementComponent = Cast<UGMCE_OrganicMovementCmp>(OwnerPawn->GetComponentByClass<UGMCE_OrganicMovementCmp>());
		}
	}

	bIsFirstUpdate = true;
}

void UGMCE_BaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// If we have no movement component, we cannot really do anything.
	if (bIsFirstUpdate && OwnerPawn && !MovementComponent)
	{
		// If the component was added in a blueprint, it may not actually be set at time of initialize, so we'll
		// try to grab it again.
		MovementComponent = Cast<UGMCE_OrganicMovementCmp>(OwnerPawn->GetComponentByClass<UGMCE_OrganicMovementCmp>());
	}

	if (!MovementComponent) return;

	WorldVelocity = MovementComponent->GetLinearVelocity_GMC();
	WorldAcceleration = MovementComponent->GetCurrentAnimationAcceleration();

	AimRotation = MovementComponent->GetCurrentAimRotation();
	AimYawDeltaRate = MovementComponent->GetCurrentAimYawRate();

	WorldRotation = MovementComponent->GetCurrentComponentRotation();
	ComponentYawDeltaRate = MovementComponent->GetCurrentComponentYawRate();

	FVector AimDirection = UKismetMathLibrary::Conv_RotatorToVector(AimRotation).GetSafeNormal2D();
	FVector WorldDirection = UKismetMathLibrary::Conv_RotatorToVector(WorldRotation).GetSafeNormal2D();

	AimYawRemaining = MovementComponent->GetAimYawRemaining();
	ComponentYawRemaining = MovementComponent->GetComponentYawRemaining();
	
	InputAcceleration = MovementComponent->GetProcessedInputVector() * MovementComponent->GetInputAcceleration();
	InputDirection = InputAcceleration.GetSafeNormal();

	AimOffset = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, WorldRotation);

	WorldGroundVelocity = WorldVelocity * FVector(1.f, 1.f, 0.f);

	LocomotionAngle = MovementComponent->GetLocomotionAngle();
	OrientationAngle = MovementComponent->GetOrientationAngle();

	bTurnInPlace = MovementComponent->IsTurningInPlace();
	
	if (GetWorld()->IsGameWorld())
	{
		const auto ComponentTransform = MovementComponent->UpdatedComponent->GetComponentTransform();
		LocalVelocity = ComponentTransform.InverseTransformVector(WorldVelocity);
		LocalGroundVelocity = LocalVelocity * FVector(1.f, 1.f, 0.f);
		LocalAcceleration = ComponentTransform.InverseTransformVector(WorldAcceleration);
		WorldLocation = MovementComponent->UpdatedComponent->GetComponentLocation();
		WorldInputDirection = MovementComponent->GetControllerRotation_GMC().RotateVector(InputDirection).GetSafeNormal2D();
		WorldInputAcceleration = MovementComponent->GetControllerRotation_GMC().RotateVector(InputAcceleration).GetSafeNormal2D();
		LocalInputDirection = ComponentTransform.InverseTransformVector(WorldInputDirection);
		LocalInputAcceleration = ComponentTransform.InverseTransformVector(WorldInputAcceleration);
	}
	else
	{
		// We're an editor preview, there's no actual world transform to bother with.
		LocalVelocity = WorldVelocity;
		LocalGroundVelocity = WorldGroundVelocity;
		LocalAcceleration = WorldAcceleration;
		WorldLocation = FVector::ZeroVector;
		WorldInputDirection = LocalInputDirection = InputDirection;
		WorldInputAcceleration = LocalInputAcceleration = InputAcceleration;
	}

	LocalVelocityDirection = LocalVelocity.GetSafeNormal();
	LocalGroundVelocityDirection = LocalGroundVelocity.GetSafeNormal();
	
	bIsMoving = !WorldVelocity.IsNearlyZero();
	bHasInput = MovementComponent->IsInputPresent();
	Speed = WorldVelocity.Length();
	GroundSpeed = MovementComponent->GetSpeedXY();
	VerticalSpeed = MovementComponent->GetSpeedZ();

	FVector PredictedStopLocation;
	bHasPredictedStop = MovementComponent->IsStopPredicted(PredictedStopLocation);
	PredictedStopDistance = bHasPredictedStop ? PredictedStopLocation.Length() : 0.f;

	FVector PredictedPivotLocation;
	bHasPredictedPivot = MovementComponent->IsPivotPredicted(PredictedPivotLocation);
	PredictedPivotDistance = bHasPredictedPivot ? PredictedPivotLocation.Length() : 0.f;

	bIsFirstUpdate = false;
}
