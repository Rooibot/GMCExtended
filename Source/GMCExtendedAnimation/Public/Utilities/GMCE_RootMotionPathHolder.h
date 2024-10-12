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
	void TestGeneratePath(AGMC_Pawn* Pawn, UAnimMontage* Montage, float StartPosition, float PlayRate);
};
