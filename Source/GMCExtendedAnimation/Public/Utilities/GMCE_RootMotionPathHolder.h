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

	UFUNCTION(BlueprintCallable)
	void GenerateMontagePath(AGMC_Pawn* Pawn, UAnimMontage* Montage, float StartPosition, float PlayRate, bool bDrawDebug = false);

	UFUNCTION(BlueprintCallable)
	void GenerateMontagePathWithOverrides(AGMC_Pawn* Pawn, UAnimMontage* Montage, float StartPosition, float PlayRate, const FTransform& OriginTransform, const FTransform& MeshRelativeTransform, bool bDrawDebug = false);
	
	FGMCE_MovementSampleCollection GetCalculatedPath() const { return PredictedSamples; }

	UFUNCTION(BlueprintCallable)
	FTransform GetTransformAtPosition(float Position);

	UFUNCTION(BlueprintCallable)
	void Reset();

	UFUNCTION(BlueprintCallable)
	void GetDeltaBetweenPositions(float StartPosition, float EndPosition, FVector& OutDelta, FVector& OutVelocity, float DeltaTimeOverride);
	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GMC Extended")
	float PredictionSampleInterval { 0.01f };
	
	FGMCE_MovementSampleCollection PredictedSamples;
	
};
