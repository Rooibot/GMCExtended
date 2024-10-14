// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GMCE_RootMotionModifier.h"
#include "UObject/Object.h"
#include "GMCE_MotionWarpSubject.h"
#include "GMCE_RootMotionPathHolder.generated.h"

class UGMCE_MotionWarpingComponent;

/**
 * 
 */
UCLASS(BlueprintType)
class GMCEXTENDEDANIMATION_API UGMCE_RootMotionPathHolder : public UObject
{
	GENERATED_BODY()

public:
	bool GeneratePathForMontage(UGMCE_MotionWarpingComponent* WarpingComponent, USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage, const FGMCE_MotionWarpContext& Context);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay="bDrawDebug"))
	void GenerateMontagePath(AGMC_Pawn* Pawn, UAnimMontage* Montage, float StartPosition, float PlayRate, bool bDrawDebug = false);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay="bDrawDebug"))
	void GenerateMontagePathWithOverrides(AGMC_Pawn* Pawn, UAnimMontage* Montage, float StartPosition, float PlayRate, const FTransform& OriginTransform, const FTransform& MeshRelativeTransform, bool bDrawDebug = false);
	
	FGMCE_MovementSampleCollection GetCalculatedPath() const { return PredictedSamples; }

	UFUNCTION(BlueprintCallable)
	bool GetTransformsAtPosition(float Position, FTransform& OutComponentTransform, FTransform& OutActorTransform);

	UFUNCTION(BlueprintCallable)
	bool GetTransformsAtPositionWithBlendOut(const UGMCE_OrganicMovementCmp* MovementComponent, float Position, FTransform& OutComponentTransform, FTransform& OutActorTransform, bool& OutShouldBlendOut, float& OutBlendOutTime);

	UFUNCTION(BlueprintCallable)
	void Reset();

	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm="OverrideOrigin", AdvancedDisplay="OverrideOrigin,bShowDebug"))
	void GetActorDeltaBetweenPositions(float StartPosition, float EndPosition, UPARAM(ref) const FVector& OverrideOrigin, FVector& OutDelta, FVector& OutVelocity, float DeltaTimeOverride, bool bShowDebug);
	
	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm="OverrideOrigin", AdvancedDisplay="OverrideOrigin,bShowDebug"))
	bool GetActorDeltaBetweenPositionsWithBlendOut(const UGMCE_OrganicMovementCmp* MovementComponent, float StartPosition, float EndPosition, UPARAM(ref) const FVector& OverrideOrigin, FVector& OutDelta, FVector& OutVelocity, float &OutBlendOutTime, float DeltaTimeOverride, bool bShowDebug);


protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GMC Extended")
	float PredictionSampleInterval { 0.01f };
	
	FGMCE_MovementSampleCollection PredictedSamples;

	UPROPERTY()
	UAnimSequenceBase* PredictionSequence;
	
};
