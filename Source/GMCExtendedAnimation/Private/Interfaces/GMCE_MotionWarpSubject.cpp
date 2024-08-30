#include "Interfaces/GMCE_MotionWarpSubject.h"


// Add default functionality here for any IGMCE_MotionWarpSubject functions that are not pure virtual.
USkeletalMeshComponent* IGMCE_MotionWarpSubject::MotionWarping_GetMeshComponent() const
{
	return nullptr;
}

float IGMCE_MotionWarpSubject::MotionWarping_GetCollisionHalfHeight() const
{
	return 0.f;
}

FQuat IGMCE_MotionWarpSubject::MotionWarping_GetRotationOffset() const
{
	return FQuat::Identity;
}

FVector IGMCE_MotionWarpSubject::MotionWarping_GetTranslationOffset() const
{
	return FVector::ZeroVector;
}

void IGMCE_MotionWarpSubject::MotionWarping_RecacheValues()
{
	
}

FAnimMontageInstance* IGMCE_MotionWarpSubject::GetRootMotionAnimMontageInstance(
	USkeletalMeshComponent* MeshComponent) const
{
	USkeletalMeshComponent* Component = MeshComponent;
	if (!Component)
	{
		Component = MotionWarping_GetMeshComponent();
	}
	
	return Component && Component->GetAnimInstance() ? Component->GetAnimInstance()->GetRootMotionMontageInstance() : nullptr;
}

UGMCE_OrganicMovementCmp* IGMCE_MotionWarpSubject::GetGMCExMovementComponent() const
{
	return nullptr;
}
