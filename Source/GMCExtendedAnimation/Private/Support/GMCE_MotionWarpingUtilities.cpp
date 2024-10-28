// Copyright 2024 Rooibot Games, LLC


#include "Support/GMCE_MotionWarpingUtilities.h"

#include "AnimNotifyState_GMCExMotionWarp.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_RootMotionModifier_Warp.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
TAutoConsoleVariable<int32> FGMCE_MotionWarpCvars::CVarMotionWarpingDisable(TEXT("a.GMCEx.MotionWarp.Disable"), 0, TEXT("Disable Motion Warping"), ECVF_Cheat);
TAutoConsoleVariable<int32> FGMCE_MotionWarpCvars::CVarMotionWarpingFromTracker(TEXT("a.GMCEx.MotionWarp.FromTracker"), 1, TEXT("Take values from GMC montage tracker versus the montage itself"), ECVF_Cheat);
TAutoConsoleVariable<int32> FGMCE_MotionWarpCvars::CVarMotionWarpingDebug(TEXT("a.GMCEx.MotionWarp.Debug"), 0, TEXT("0: Disable, 1: Only Log, 2: Only DrawDebug, 3: Log and DrawDebug"), ECVF_Cheat);
TAutoConsoleVariable<float> FGMCE_MotionWarpCvars::CVarMotionWarpingDrawDebugDuration(TEXT("a.GMCEx.MotionWarp.DrawDebugLifeTime"), 1.f, TEXT("Time in seconds each draw debug persists.\nRequires 'a.MotionWarping.Debug 2'"), ECVF_Cheat);
#endif


void UGMCE_MotionWarpingUtilities::ExtractLocalSpacePose(const UAnimSequenceBase* Animation,
                                                         const FBoneContainer& BoneContainer, float Time, bool bExtractRootMotion, FCompactPose& OutPose)
{
	OutPose.SetBoneContainer(&BoneContainer);

	FBlendedCurve Curve;
	Curve.InitFrom(BoneContainer);

	FAnimExtractContext Context(static_cast<double>(Time), bExtractRootMotion);

	UE::Anim::FStackAttributeContainer Attributes;
	FAnimationPoseData AnimationPoseData(OutPose, Curve, Attributes);
	if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Animation))
	{
		AnimSequence->GetBonePose(AnimationPoseData, Context);
	}
	else if (const UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation))
	{
		const FAnimTrack& AnimTrack = AnimMontage->SlotAnimTracks[0].AnimTrack;
		AnimTrack.GetAnimationPose(AnimationPoseData, Context);
	}	
}

void UGMCE_MotionWarpingUtilities::ExtractComponentSpacePose(const UAnimSequenceBase* Animation,
	const FBoneContainer& BoneContainer, float Time, bool bExtractRootMotion, FCSPose<FCompactPose>& OutPose)
{
	FCompactPose Pose;
	ExtractLocalSpacePose(Animation, BoneContainer, Time, bExtractRootMotion, Pose);
	OutPose.InitPose(MoveTemp(Pose));	
}

FTransform UGMCE_MotionWarpingUtilities::ExtractRootMotionFromAnimation(const UAnimSequenceBase* Animation,
	float StartTime, float EndTime)
{
	if (const UAnimMontage* Anim = Cast<UAnimMontage>(Animation))
	{
		// This is identical to UAnimMontage::ExtractRootMotionFromTrackRange and UAnimCompositeBase::ExtractRootMotionFromTrack but ignoring bEnableRootMotion
		// so we can extract root motion from the montage even if that flag is set to false in the AnimSequence(s)

		FRootMotionMovementParams AccumulatedRootMotionParams;

		if (Anim->SlotAnimTracks.Num() > 0)
		{
			const FAnimTrack& RootMotionAnimTrack = Anim->SlotAnimTracks[0].AnimTrack;

			TArray<FRootMotionExtractionStep> RootMotionExtractionSteps;
			RootMotionAnimTrack.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, StartTime, EndTime);

			for (const FRootMotionExtractionStep& CurStep : RootMotionExtractionSteps)
			{
				if (CurStep.AnimSequence)
				{
					AccumulatedRootMotionParams.Accumulate(CurStep.AnimSequence->ExtractRootMotionFromRange(CurStep.StartPosition, CurStep.EndPosition));
				}
			}
		}

		return AccumulatedRootMotionParams.GetRootMotionTransform();
	}

	if (const UAnimSequence* Anim = Cast<UAnimSequence>(Animation))
	{
		return Anim->ExtractRootMotionFromRange(StartTime, EndTime);
	}

	return FTransform::Identity;	
}

FTransform UGMCE_MotionWarpingUtilities::ExtractRootTransformFromAnimation(const UAnimSequenceBase* Animation,
	float Time)
{
	if (const UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation))
	{
		if(const FAnimSegment* Segment = AnimMontage->SlotAnimTracks[0].AnimTrack.GetSegmentAtTime(Time))
		{
			if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Segment->GetAnimReference()))
			{
				const float AnimSequenceTime = Segment->ConvertTrackPosToAnimPos(Time);
				return AnimSequence->ExtractRootTrackTransform(AnimSequenceTime, nullptr);
			}	
		}
	}
	else if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Animation))
	{
		return AnimSequence->ExtractRootTrackTransform(Time, nullptr);
	}

	return FTransform::Identity;
}

void UGMCE_MotionWarpingUtilities::GetMotionWarpingWindowsFromAnimation(const UAnimSequenceBase* Animation,
	TArray<FGMCE_MotionWarpingWindowData>& OutWindows)
{
	if(Animation)
	{
		OutWindows.Reset();

		for (int32 Idx = 0; Idx < Animation->Notifies.Num(); Idx++)
		{
			const FAnimNotifyEvent& NotifyEvent = Animation->Notifies[Idx];
			if (UAnimNotifyState_GMCExMotionWarp* Notify = Cast<UAnimNotifyState_GMCExMotionWarp>(NotifyEvent.NotifyStateClass))
			{
				FGMCE_MotionWarpingWindowData Data;
				Data.AnimNotify = Notify;
				Data.StartTime = NotifyEvent.GetTriggerTime();
				Data.EndTime = NotifyEvent.GetEndTriggerTime();
				OutWindows.Add(Data);
			}
		}
	}	
}

void UGMCE_MotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(const UAnimSequenceBase* Animation,
	FName WarpTargetName, TArray<FGMCE_MotionWarpingWindowData>& OutWindows)
{
	if (Animation && WarpTargetName != NAME_None)
	{
		OutWindows.Reset();

		for (int32 Idx = 0; Idx < Animation->Notifies.Num(); Idx++)
		{
			const FAnimNotifyEvent& NotifyEvent = Animation->Notifies[Idx];
			if (UAnimNotifyState_GMCExMotionWarp* Notify = Cast<UAnimNotifyState_GMCExMotionWarp>(NotifyEvent.NotifyStateClass))
			{
				if (const UGMCE_RootMotionModifier_Warp* Modifier = Cast<const UGMCE_RootMotionModifier_Warp>(Notify->RootMotionModifier))
				{
					if(Modifier->WarpTargetName == WarpTargetName)
					{
						FGMCE_MotionWarpingWindowData Data;
						Data.AnimNotify = Notify;
						Data.StartTime = NotifyEvent.GetTriggerTime();
						Data.EndTime = NotifyEvent.GetEndTriggerTime();
						OutWindows.Add(Data);
					}
				}
			}
		}
	}	
}

FTransform UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(
	const FTransform& RelativeTransform, const UAnimInstance* AnimInstance, const UAnimSequenceBase* Animation,
	float Time, const FName& WarpPointBoneName)
{
	const FBoneContainer& FullBoneContainer = AnimInstance->GetRequiredBones();
	const int32 BoneIndex = FullBoneContainer.GetPoseBoneIndexForBoneName(WarpPointBoneName);
	if (BoneIndex != INDEX_NONE && BoneIndex != 0)
	{
		TArray<FBoneIndexType> RequiredBoneIndexArray = { 0, (FBoneIndexType)BoneIndex };
		FullBoneContainer.GetReferenceSkeleton().EnsureParentsExistAndSort(RequiredBoneIndexArray);

		FBoneContainer LimitedBoneContainer(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(UE::Anim::ECurveFilterMode::DisallowAll), *FullBoneContainer.GetAsset());

		FCSPose<FCompactPose> Pose;
		ExtractComponentSpacePose(Animation, LimitedBoneContainer, Time, false, Pose);

		// Inverse of mesh's relative rotation. Used to convert root and warp point in the animation from Y forward to X forward
		const FTransform MeshCompRelativeRotInverse = FTransform(RelativeTransform.GetRotation().Inverse());

		const FTransform RootTransform = MeshCompRelativeRotInverse * Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0));
		const FTransform WarpPointTransform = MeshCompRelativeRotInverse * Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(1));
		return RootTransform.GetRelativeTransform(WarpPointTransform);
	}

	return FTransform::Identity;
}

FTransform UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(
	const FTransform& RelativeTransform, const UAnimSequenceBase* Animation, float Time,
	const FTransform& WarpPointTransform)
{
	// Inverse of mesh's relative rotation. Used to convert root and warp point in the animation from Y forward to X forward
	const FTransform MeshCompRelativeRotInverse = FTransform(RelativeTransform.GetRotation().Inverse());
	const FTransform RootTransform = MeshCompRelativeRotInverse * ExtractRootTransformFromAnimation(Animation, Time);
	return RootTransform.GetRelativeTransform((MeshCompRelativeRotInverse * WarpPointTransform));	
}

FTransform UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(AGMC_Pawn* Character,
                                                                                         const UAnimSequenceBase* Animation, float Time, const FName& WarpPointBoneName)
{
	IGMCE_MotionWarpSubject* MotionWarpInterface =Cast<IGMCE_MotionWarpSubject>(Character);
	if (const USkeletalMeshComponent* Mesh = MotionWarpInterface->MotionWarping_GetMeshComponent())
	{
		if (const UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			return CalculateRootTransformRelativeToWarpPointAtTime(FTransform(MotionWarpInterface->MotionWarping_GetRotationOffset()),
				AnimInstance, Animation, Time, WarpPointBoneName);
		}
	}

	return FTransform::Identity;	
}

FTransform UGMCE_MotionWarpingUtilities::CalculateRootTransformRelativeToWarpPointAtTime(AGMC_Pawn* Character,
	const UAnimSequenceBase* Animation, float Time, const FTransform& WarpPointTransform)
{
	IGMCE_MotionWarpSubject* MotionWarpInterface =Cast<IGMCE_MotionWarpSubject>(Character);
	return CalculateRootTransformRelativeToWarpPointAtTime(FTransform(MotionWarpInterface->MotionWarping_GetRotationOffset()), Animation, Time, WarpPointTransform);
}
