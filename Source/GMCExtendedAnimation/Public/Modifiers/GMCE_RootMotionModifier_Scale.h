// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "GMCE_RootMotionModifier.h"
#include "GMCE_RootMotionModifier_Scale.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName="Scale"))
class GMCEXTENDEDANIMATION_API UGMCE_RootMotionModifier_Scale : public UGMCE_RootMotionModifier
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
	FVector Scale { 1.f };

	UGMCE_RootMotionModifier_Scale(const FObjectInitializer& ObjectInitializer);

	virtual FTransform ProcessRootMotion(const FTransform& InRootMotion, const FGMCE_MotionWarpContext& WarpContext) override
	{
		FTransform FinalRootMotion = InRootMotion;
		FinalRootMotion.ScaleTranslation(Scale);
		return FinalRootMotion;
	}

	UFUNCTION(BlueprintCallable, Category = "Motion Warping")
	static UGMCE_RootMotionModifier_Scale* AddRootMotionModifierScale(
		UPARAM(DisplayName = "Motion Warping Comp") UGMCE_MotionWarpingComponent* InMotionWarpingComp,
		UPARAM(DisplayName = "Animation") const UAnimSequenceBase* InAnimation,
		UPARAM(DisplayName = "Start Time") float InStartTime,
		UPARAM(DisplayName = "End Time") float InEndTime,
		UPARAM(DisplayName = "Scale") FVector InScale = FVector(1.f));	
};
