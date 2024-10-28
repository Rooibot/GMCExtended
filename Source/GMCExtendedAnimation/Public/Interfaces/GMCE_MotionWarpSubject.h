#pragma once

#include "CoreMinimal.h"
#include "GMCE_OrganicMovementCmp.h"
#include "UObject/Interface.h"
#include "GMCE_MotionWarpSubject.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGMCE_MotionWarpSubject : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GMCEXTENDEDANIMATION_API IGMCE_MotionWarpSubject
{
	GENERATED_BODY()

public:
	virtual USkeletalMeshComponent* MotionWarping_GetMeshComponent() const;
	virtual float MotionWarping_GetCollisionHalfHeight() const;
	virtual FQuat MotionWarping_GetRotationOffset() const;
	virtual FVector MotionWarping_GetTranslationOffset() const;
	virtual FAnimMontageInstance* GetRootMotionAnimMontageInstance(USkeletalMeshComponent* MeshComponent) const;
	virtual UGMCE_OrganicMovementCmp* GetGMCExMovementComponent() const;
	virtual void MotionWarping_RecacheValues();
};
