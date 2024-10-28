// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "GMCPawn.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GMCE_MotionWarpingUtilities.generated.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
struct FGMCE_MotionWarpContext;

struct GMCEXTENDEDANIMATION_API FGMCE_MotionWarpCvars
{
	static TAutoConsoleVariable<int32> CVarMotionWarpingDisable;
	static TAutoConsoleVariable<int32> CVarMotionWarpingFromTracker;
	static TAutoConsoleVariable<int32> CVarMotionWarpingDebug;
	static TAutoConsoleVariable<float> CVarMotionWarpingDrawDebugDuration;
};
#endif

/**
 * 
 */
UCLASS()
class GMCEXTENDEDANIMATION_API UGMCE_MotionWarpingUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Extract bone pose in local space for all bones in BoneContainer. If Animation is a Montage the pose is extracted from the first track */
	static void ExtractLocalSpacePose(const UAnimSequenceBase* Animation, const FBoneContainer& BoneContainer, float Time, bool bExtractRootMotion, FCompactPose& OutPose);

	/** Extract bone pose in component space for all bones in BoneContainer. If Animation is a Montage the pose is extracted from the first track */
	static void ExtractComponentSpacePose(const UAnimSequenceBase* Animation, const FBoneContainer& BoneContainer, float Time, bool bExtractRootMotion, FCSPose<FCompactPose>& OutPose);

	/** Extract Root Motion transform from a contiguous position range */
	UFUNCTION(BlueprintCallable, Category = "Motion Warping")
	static FTransform ExtractRootMotionFromAnimation(const UAnimSequenceBase* Animation, float StartTime, float EndTime);

	/** Extract root bone transform at a given time */
	static FTransform ExtractRootTransformFromAnimation(const UAnimSequenceBase* Animation, float Time);

	/** @return All the MotionWarping windows within the supplied animation */
	UFUNCTION(BlueprintCallable, Category = "Motion Warping")
	static void GetMotionWarpingWindowsFromAnimation(const UAnimSequenceBase* Animation, TArray<FGMCE_MotionWarpingWindowData>& OutWindows);

	/** @return All the MotionWarping windows within the supplied animation for a given Warp Target */
	UFUNCTION(BlueprintCallable, Category = "Motion Warping")
	static void GetMotionWarpingWindowsForWarpTargetFromAnimation(const UAnimSequenceBase* Animation, FName WarpTargetName, TArray<FGMCE_MotionWarpingWindowData>& OutWindows);

	/** @return root transform relative to the warp point bone at the supplied time */
	static FTransform CalculateRootTransformRelativeToWarpPointAtTime(const FTransform& RelativeTransform, const UAnimInstance* AnimInstance, const UAnimSequenceBase* Animation, float Time, const FName& WarpPointBoneName);
	
	/** @return root transform relative to the warp point bone at the supplied time */
	static FTransform CalculateRootTransformRelativeToWarpPointAtTime(AGMC_Pawn* Character, const UAnimSequenceBase* Animation, float Time, const FName& WarpPointBoneName);

	/** @return root transform relative to the warp point transform at the supplied time */
	static FTransform CalculateRootTransformRelativeToWarpPointAtTime(const FTransform& RelativeTransform, const UAnimSequenceBase* Animation, float Time, const FTransform& WarpPointTransform);
	
	/** @return root transform relative to the warp point transform at the supplied time */
	static FTransform CalculateRootTransformRelativeToWarpPointAtTime(AGMC_Pawn* Character, const UAnimSequenceBase* Animation, float Time, const FTransform& WarpPointTransform);	
};
