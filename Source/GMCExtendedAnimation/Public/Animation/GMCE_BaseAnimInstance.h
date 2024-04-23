#pragma once

#include "CoreMinimal.h"
#include "GMCE_OrganicMovementCmp.h"
#include "GMCPawn.h"
#include "Animation/AnimInstance.h"
#include "GMCE_BaseAnimInstance.generated.h"

/**
 * 
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
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character")
	TObjectPtr<AGMC_Pawn> OwnerPawn;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character")
	TObjectPtr<UGMCE_OrganicMovementCmp> MovementComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	bool bIsFirstUpdate { true };
	
	// Character Movement State
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	bool bIsMoving { false };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	bool bHasInput { false };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector WorldLocation { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FRotator WorldRotation{ 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FRotator AimRotation { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FRotator AimOffset { 0.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	float AimYawDeltaRate { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	float AimYawRemaining { 0.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	float ComponentYawDeltaRate { 0.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector WorldVelocity { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector LocalVelocity { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector LocalGroundVelocity { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector LocalVelocityDirection { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector LocalGroundVelocityDirection { 0.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector WorldAcceleration { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector LocalAcceleration { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector InputDirection { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector InputAcceleration { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector WorldInputDirection { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector WorldInputAcceleration { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector LocalInputDirection { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	FVector LocalInputAcceleration { 0.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	float Speed { 0.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	float GroundSpeed { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|State")
	float VerticalSpeed { 0.f };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Pivot")
	bool bHasPredictedPivot { false };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Pivot")
	float PredictedPivotDistance { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Stop")
	bool bHasPredictedStop { false };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Stop")
	float PredictedStopDistance { 0.f };
	
};
