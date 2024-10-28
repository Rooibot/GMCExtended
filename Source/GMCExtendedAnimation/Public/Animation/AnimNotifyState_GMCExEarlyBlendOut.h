// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_GMCExEarlyBlendOut.generated.h"

UENUM(BlueprintType)
enum class EGMCExEarlyBlendOutCondition : uint8
{
	Forced,
	OnInput,
	OnFalling
};

class UGMCE_OrganicMovementCmp;

/**
 * 
 */
UCLASS(DisplayName = "GMCEx Early Blend Out", BlueprintType)
class GMCEXTENDEDANIMATION_API UAnimNotifyState_GMCExEarlyBlendOut : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	UFUNCTION()
	bool ShouldBlendOut(const UGMCE_OrganicMovementCmp* MovementComponent, bool bIsPredicted = false) const;

	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGMCExEarlyBlendOutCondition Condition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlendOutTime;
};
