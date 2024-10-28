#include "Actors/GMCE_MotionWarpPawn.h"


// Sets default values
AGMCE_MotionWarpPawn::AGMCE_MotionWarpPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

USkeletalMeshComponent* AGMCE_MotionWarpPawn::MotionWarping_GetMeshComponent() const
{
	return MotionWarp_SkeletalMesh;
}

float AGMCE_MotionWarpPawn::MotionWarping_GetCollisionHalfHeight() const
{
	if (!MotionWarp_MovementComponent) return 0.f;

	return MotionWarp_MovementComponent->GetRootCollisionHalfHeight(true);
}

FQuat AGMCE_MotionWarpPawn::MotionWarping_GetRotationOffset() const
{
	if (!MotionWarp_SkeletalMesh) return FQuat::Identity;
	
	return MotionWarp_SkeletalMesh->GetRelativeRotation().Quaternion();
}

FVector AGMCE_MotionWarpPawn::MotionWarping_GetTranslationOffset() const
{
	if (!MotionWarp_SkeletalMesh) return FVector::ZeroVector;

	return MotionWarp_SkeletalMesh->GetRelativeLocation();
}

void AGMCE_MotionWarpPawn::MotionWarping_RecacheValues()
{
	// Search for (and cache) our skeletal mesh and movement component.
	MotionWarp_SkeletalMesh = FindComponentByClass<USkeletalMeshComponent>();
	MotionWarp_MovementComponent = FindComponentByClass<UGMCE_OrganicMovementCmp>();	
}

FAnimMontageInstance* AGMCE_MotionWarpPawn::GetRootMotionAnimMontageInstance(
	USkeletalMeshComponent* MeshComponent) const
{
	// We can just use the default here.
	return IGMCE_MotionWarpSubject::GetRootMotionAnimMontageInstance(MeshComponent);
}

UGMCE_OrganicMovementCmp* AGMCE_MotionWarpPawn::GetGMCExMovementComponent() const
{
	return MotionWarp_MovementComponent;
}

void AGMCE_MotionWarpPawn::BeginPlay()
{
	Super::BeginPlay();
	MotionWarping_RecacheValues();
}

