#pragma once

#include "CoreMinimal.h"
#include "GMCE_MotionWarpSubject.h"
#include "GMCPawn.h"
#include "GMCE_MotionWarpPawn.generated.h"

/**
 * A convenience version of the GMC Pawn which has implementations of the GMCEx motion warp subject
 * interface already built in. You will need to add both a GMCEx MotionWarpingComponent and some variant
 * of GMCEx's Organic Movement Component to any pawn built atop this for motion warping to work.
 */
UCLASS(ClassGroup=(GMCExtended), meta=(DisplayName="GMCEx Motion Warp Pawn"))
class GMCEXTENDEDANIMATION_API AGMCE_MotionWarpPawn : public AGMC_Pawn, public IGMCE_MotionWarpSubject
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGMCE_MotionWarpPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual USkeletalMeshComponent* MotionWarping_GetMeshComponent() const override;
	virtual float MotionWarping_GetCollisionHalfHeight() const override;
	virtual FQuat MotionWarping_GetRotationOffset() const override;
	virtual FVector MotionWarping_GetTranslationOffset() const override;
	virtual void MotionWarping_RecacheValues() override;
	virtual FAnimMontageInstance* GetRootMotionAnimMontageInstance(USkeletalMeshComponent* MeshComponent) const override;
	virtual UGMCE_OrganicMovementCmp* GetGMCExMovementComponent() const override;

protected:

	virtual void BeginPlay() override;
	
	// Cached objects
	UPROPERTY()
	USkeletalMeshComponent* MotionWarp_SkeletalMesh { nullptr };

	UPROPERTY()
	UGMCE_OrganicMovementCmp* MotionWarp_MovementComponent { nullptr };
	
};
