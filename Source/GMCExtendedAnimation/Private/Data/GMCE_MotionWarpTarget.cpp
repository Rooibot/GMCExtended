#include "GMCE_MotionWarpTarget.h"
#include "GMCExtendedAnimation.h"
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
	const FTransform& ComponentRelative, const UAnimSequenceBase* Animation, float Timestamp) const
{
	if (!bFollowComponent)
	{
		return FTransform(Rotation, Location);
	}

	const FTransform ComponentToWorld = Origin * ComponentRelative;
	
	if (BoneName != NAME_None)
	{
		return GetTargetTransformFromAnimation(Animation, Timestamp, BoneName, ComponentToWorld);
	}

	return ComponentToWorld;
}

FTransform FGMCE_MotionWarpTarget::GetTargetTransformFromAnimation(const UAnimSequenceBase* Animation, float Timestamp,
	const FName& BoneName, const FTransform& ComponentToWorld)
{
	IAnimationDataModel* Model = Animation->GetDataModel();
	const int32 FrameNum = Animation->GetFrameAtTime(Timestamp);
	const float FramePos = Animation->GetTimeAtFrame(FrameNum);
	const FFrameTime FrameTime = FFrameTime(FrameNum, Timestamp - FramePos);
	
	const FTransform BoneTrackTransform = Model->EvaluateBoneTrackTransform(BoneName, FrameTime, EAnimInterpolationType::Linear);

	return BoneTrackTransform * ComponentToWorld;
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
