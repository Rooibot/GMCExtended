#pragma once

#include "CoreMinimal.h"
#include "GMCE_OrganicMovementCmp.h"
#include "GMCPawn.h"
#include "Animation/AnimInstance.h"
#include "GMCE_BaseAnimInstance.generated.h"

/**
 * This is a simple animation blueprint parent meant for use with the GMC Extended framework. Basically, it will
 * precalculate and make available a number of useful movement-related values. It does *not*, however, handle
 * any actual movement logic. If you wish to use an animation blueprint system which handles a lot of the logic
 * for you, the GMC Extended Movement Animation Blueprint is where you want to look; it is part of the overall
 * prebuilt animation movement system.
 */
UCLASS(meta=(DisplayName="GMC Extended Animation Blueprint"))
class GMCEXTENDEDANIMATION_API UGMCE_BaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	
	// We read values from the movement component on the game thread in the update.
	// They can then be safely accessed via Property Access on the thread-safe animation
	// update thread.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Configuration

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Preview")
	TSubclassOf<AGMC_Pawn> EditorPreviewClass { AGMC_Pawn::StaticClass() };
#endif
	
	// General Character Components 
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Character")
	TObjectPtr<AGMC_Pawn> OwnerPawn { nullptr };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Character")
	TObjectPtr<UGMCE_OrganicMovementCmp> MovementComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Default")
	bool bIsFirstUpdate { true };

	// -- Useful state.
	
	/// Whether the character is currently moving.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement")
	bool bIsMoving { false };

	/// Whether user input is present or not.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement")
	bool bHasInput { false };

	/// The character's current position in world space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Location")
	FVector WorldLocation { 0.f };

	/// The character's current rotation in world space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Location")
	FRotator WorldRotation{ 0.f };

	/// The character's current aim rotation, in world space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Aiming")
	FRotator AimRotation { 0.f };

	/// The character's current aim rotation as an offset from the character's rotation.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Aiming")
	FRotator AimOffset { 0.f };

	/// The rate at which the aim rotation is currently changing.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Aiming")
	float AimYawDeltaRate { 0.f };

	/// The degrees of yaw remaining between the aim rotation and the character's current rotation.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	float AimYawRemaining { 0.f };

	/// Whether the character should be turning in place or not.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	bool bTurnInPlace { false };

	/// The rate at which the character mesh is rotating for a turn.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	float ComponentYawDeltaRate { 0.f };

	/// The degrees of yaw remaining between the target turn-in-place rotation and the character's current rotation.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Turn-in-Place")
	float ComponentYawRemaining { 0.f };

	/// The character's current velocity in world space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Velocity")
	FVector WorldVelocity { 0.f };

	/// The character's current velocity in world space, minus any vertical component.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Velocity")
	FVector WorldGroundVelocity { 0.f };

	/// The character's current velocity in local space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Velocity")
	FVector LocalVelocity { 0.f };

	/// The character's current velocity in local space, minus any vertical component.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Velocity")
	FVector LocalGroundVelocity { 0.f };

	/// The direction the character is moving in, in local space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Velocity")
	FVector LocalVelocityDirection { 0.f };

	/// The direction the character is moving in, in local space, minus any vertical component.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Velocity")
	FVector LocalGroundVelocityDirection { 0.f };

	/// The character's current effective acceleration, in world space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Acceleration")
	FVector WorldAcceleration { 0.f };

	/// The character's current effective acceleration, in local space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Acceleration")
	FVector LocalAcceleration { 0.f };

	/// The direction player input is being applied in, purely in the form taken from the input device.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Input")
	FVector InputDirection { 0.f };

	/// The effective acceleration being applied by player input. THIS MAY NOT MATCH THE ACTUAL ACCELERATION.
	/// Actual acceleration is computed from the character's movement history.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Input")
	FVector InputAcceleration { 0.f };

	/// The direction player input is being applied, in world space.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Input")
	FVector WorldInputDirection { 0.f };

	/// The effective acceleration being applied by player input, in world space. THIS MAY NOT MATCH THE
	/// ACTUAL ACCELERATION. Actual acceleration is computed from the character's movement history.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Input")
	FVector WorldInputAcceleration { 0.f };

	/// The direction player input is being applied, relative to the character's own orientation.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Input")
	FVector LocalInputDirection { 0.f };

	/// The effective acceleration being applied by player input, relative to the character's own
	/// orientation. THIS MAY NOT MATCH THE ACTUAL ACCELERATION. Actual acceleration is computed from
	/// the character's movement history.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement|Input")
	FVector LocalInputAcceleration { 0.f };

	/// The speed at which the character is moving.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement")
	float Speed { 0.f };

	/// The speed at which the character is moving, minus any vertical movement.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement")
	float GroundSpeed { 0.f };

	/// The speed at which the character is moving in a vertical-only direction.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movement")
	float VerticalSpeed { 0.f };

	/// The angle the character is moving at, in the range [-180, 180]. 0 means moving directly forward.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	float LocomotionAngle { 0.f };

	/// The angle the character is facing while moving, in the range [-90, 90]. This value is suitable for
	/// passing into an Orientation Warping node even if the character is actually playing a 'walk-backwards' animation.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Orientation")
	float OrientationAngle { 0.f };

	/// True if we've predicted a spot where the character is going to pivot.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Distance Matching")
	bool bHasPredictedPivot { false };

	/// The distance to a predicted pivot.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Distance Matching")
	float PredictedPivotDistance { 0.f };

	/// True if we've predicted a location where the character will stop.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Distance Matching")
	bool bHasPredictedStop { false };

	/// The distance to a predicted stop.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Distance Matching")
	float PredictedStopDistance { 0.f };
	
};
