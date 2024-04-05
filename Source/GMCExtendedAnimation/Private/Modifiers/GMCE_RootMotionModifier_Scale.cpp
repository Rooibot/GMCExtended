// Copyright 2024 Rooibot Games, LLC


#include "GMCE_RootMotionModifier_Scale.h"

#include "GMCE_MotionWarpingComponent.h"

UGMCE_RootMotionModifier_Scale::UGMCE_RootMotionModifier_Scale(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

UGMCE_RootMotionModifier_Scale* UGMCE_RootMotionModifier_Scale::AddRootMotionModifierScale(
	UGMCE_MotionWarpingComponent* InMotionWarpingComp, const UAnimSequenceBase* InAnimation, float InStartTime,
	float InEndTime, FVector InScale)
{
	if (ensureAlways(InMotionWarpingComp))
	{
		UGMCE_RootMotionModifier_Scale* NewModifier = NewObject<UGMCE_RootMotionModifier_Scale>(InMotionWarpingComp);
		NewModifier->AnimationSequence = InAnimation;
		NewModifier->StartTime = InStartTime;
		NewModifier->EndTime = InEndTime;
		NewModifier->Scale = InScale;

		InMotionWarpingComp->AddModifier(NewModifier);

		return NewModifier;
	}

	return nullptr;
}
