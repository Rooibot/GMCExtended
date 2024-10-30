#include "GMCE_MotionWarpTarget.h"
#include "GMCExtendedAnimationLog.h"

FGMCE_MotionWarpTarget::FGMCE_MotionWarpTarget(const FName& InName, const USceneComponent* InComp, FName InBoneName,
                                               bool bInbFollowComponent)
{
	if (ensure(InComp))
	{
		Name = InName;
		Component = InComp;
		BoneName = InBoneName;
		bFollowComponent = bInbFollowComponent;

		FTransform Transform = FTransform::Identity;
		if (BoneName != NAME_None)
		{
			Transform = GetTargetTransformFromComponent(InComp, BoneName);
		}
		else
		{
			Transform = Component->GetComponentTransform();
		}

		Location = Transform.GetLocation();
		Rotation = Transform.Rotator();
	}
}

FTransform FGMCE_MotionWarpTarget::GetTargetTransform() const
{
	if (Component.IsValid() && bFollowComponent)
	{
		if (BoneName != NAME_None)
		{
			return GetTargetTransformFromComponent(Component.Get(), BoneName);
		}
		else
		{
			return Component->GetComponentTransform();
		}
	}

	return FTransform(Rotation, Location);	
}

FTransform FGMCE_MotionWarpTarget::GetTargetTransformFromAnimation(const FTransform& Origin,
	const FTransform& ComponentRelative, const UAnimInstance* AnimInstance, const UAnimSequenceBase* Animation, float Timestamp) const
{
	if (!bFollowComponent)
	{
		return FTransform(Rotation, Location);
	}

	const FTransform ComponentToWorld = Origin * ComponentRelative;
	
	if (BoneName != NAME_None)
	{
		return GetTargetTransformFromAnimation(AnimInstance, Animation, Timestamp, BoneName, ComponentToWorld);
	}

	return ComponentToWorld;
}

FTransform FGMCE_MotionWarpTarget::GetTargetTransformFromAnimation(const UAnimInstance* AnimInstance,
	const UAnimSequenceBase* Animation, float Timestamp, const FName& BoneName, const FTransform& ComponentToWorld)
{
	const FBoneContainer& BoneContainer = AnimInstance->GetRequiredBonesOnAnyThread();
	const int32 BoneIndex = BoneContainer.GetPoseBoneIndexForBoneName(BoneName);
	if (BoneIndex == INDEX_NONE) return FTransform::Identity;
	
	TArray<FBoneIndexType> BoneIndices;
	BoneIndices.Add(BoneIndex);
	BoneContainer.GetReferenceSkeleton().EnsureParentsExistAndSort(BoneIndices);

	FBoneContainer RequiredBones(BoneIndices, UE::Anim::FCurveFilterSettings(UE::Anim::ECurveFilterMode::DisallowAll), *BoneContainer.GetAsset());

	FCompactPose FinalPose;
	FinalPose.SetBoneContainer(&RequiredBones);

	FBlendedCurve Curve;
	Curve.InitFrom(BoneContainer);

	const FAnimExtractContext Context(static_cast<double>(Timestamp), true);

	UE::Anim::FStackAttributeContainer Attributes;
	FAnimationPoseData AnimationPoseData(FinalPose, Curve, Attributes);
	if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Animation))
	{
		AnimSequence->GetBonePose(AnimationPoseData, Context);
	}
	else if (const UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation))
	{
		const FAnimTrack& AnimTrack = AnimMontage->SlotAnimTracks[0].AnimTrack;
		AnimTrack.GetAnimationPose(AnimationPoseData, Context);
	}	

	return FinalPose.GetBones()[0] * ComponentToWorld;
}

FTransform FGMCE_MotionWarpTarget::GetTargetTransformFromComponent(const USceneComponent* Comp, const FName& BoneName)
{
	if (Comp == nullptr)
	{
		UE_LOG(LogGMCExAnimation, Warning, TEXT("FGMCE_MotionWarpTarget::GetTargetTransformFromComponent: Invalid Component"));
		return FTransform::Identity;
	}

	if (Comp->DoesSocketExist(BoneName) == false)
	{
		UE_LOG(LogGMCExAnimation, Warning, TEXT("FGMCE_MotionWarpTarget::GetTargetTransformFromComponent: Invalid Bone or Socket. Comp: %s Owner: %s BoneName: %s"),
			*GetNameSafe(Comp), *GetNameSafe(Comp->GetOwner()), *BoneName.ToString());

		return Comp->GetComponentTransform();
	}

	return Comp->GetSocketTransform(BoneName);	
}
