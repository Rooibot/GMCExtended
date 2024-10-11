#pragma once

#include "CoreMinimal.h"
#include "GMCE_RootMotionModifier_Warp.h"
#include "GMCE_RootMotionModifier_SkewWarp.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName="Skew Warp"))
class GMCEXTENDEDANIMATION_API UGMCE_RootMotionModifier_SkewWarp : public UGMCE_RootMotionModifier_Warp
{
	GENERATED_BODY()

public:
	UGMCE_RootMotionModifier_SkewWarp(const FObjectInitializer& ObjectInitializer);

	virtual FTransform ProcessRootMotion(const FTransform& InRootMotion, const FGMCE_MotionWarpContext& WarpContext) override;

	static FVector WarpTranslation(const FTransform& CurrentTransform, const FVector& DeltaTranslation, const FVector& TotalTranslation, const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Motion Warping")
	static UGMCE_RootMotionModifier_SkewWarp* AddRootMotionModifierSkewWarp(
		UPARAM(DisplayName = "Motion Warping Comp") UGMCE_MotionWarpingComponent* InMotionWarpingComp,
		UPARAM(DisplayName = "Animation") const UAnimSequenceBase* InAnimation,
		UPARAM(DisplayName = "Start Time") float InStartTime,
		UPARAM(DisplayName = "End Time") float InEndTime,
		UPARAM(DisplayName = "Warp Target Name") FName InWarpTargetName,
		UPARAM(DisplayName = "Warp Point Anim Provider") EGMCE_MotionWarpProvider InWarpPointAnimProvider,
		UPARAM(DisplayName = "Warp Point Anim Transform") FTransform InWarpPointAnimTransform,
		UPARAM(DisplayName = "Warp Point Anim Bone Name") FName InWarpPointAnimBoneName,
		UPARAM(DisplayName = "Warp Translation") bool bInWarpTranslation = true,
		UPARAM(DisplayName = "Ignore Z Axis") bool bInIgnoreZAxis = true,
		UPARAM(DisplayName = "Warp Rotation") bool bInWarpRotation = true,
		UPARAM(DisplayName = "Rotation Type") EGMCE_MotionWarpRotationType InRotationType = EGMCE_MotionWarpRotationType::Default,
		UPARAM(DisplayName = "Rotation Method") EGMCE_MotionWarpRotationMethod InRotationMethod = EGMCE_MotionWarpRotationMethod::Slerp,
		UPARAM(DisplayName = "Warp Rotation Time Multiplier") float InWarpRotationTimeMultiplier = 1.f,
		UPARAM(DisplayName = "Warp Max Rotation Rate") float InWarpMaxRotationRate = 0.f);	
};
