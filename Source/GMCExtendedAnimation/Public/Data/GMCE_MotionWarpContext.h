// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GMCE_MotionWarpContext.generated.h"

USTRUCT()
struct GMCEXTENDEDANIMATION_API FGMCE_MotionWarpContext
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<const UAnimSequenceBase> Animation { nullptr };

	UPROPERTY()
	float PreviousPosition { 0.f };

	UPROPERTY()
	float CurrentPosition { 0.f };

	UPROPERTY()
	float Weight { 0.f };

	UPROPERTY()
	float PlayRate { 0.f };

	UPROPERTY()
	float DeltaSeconds { 0.f };

	UPROPERTY()
	FTransform OwnerTransform { FTransform::Identity };

	UPROPERTY()
	FTransform MeshRelativeTransform { FTransform::Identity };

	UPROPERTY()
	UAnimInstance* AnimationInstance { nullptr };

	UPROPERTY()
	float CapsuleHalfHeight { 0.f };
	
};

