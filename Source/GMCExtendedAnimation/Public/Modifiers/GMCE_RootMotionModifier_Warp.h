// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "GMCE_RootMotionModifier.h"
#include "GMCE_RootMotionModifier_Warp.generated.h"

UENUM(BlueprintType)
enum class EGMCE_MotionWarpRotationType : uint8
{
	Default,
	Facing
};

UENUM(BlueprintType)
enum class EGMCE_MotionWarpRotationMethod : uint8
{
	Slerp,
	SlerpWithClampedRate,
	ConstantRate
};

UENUM(BlueprintType)
enum class EGMCE_MotionWarpProvider : uint8
{
	None,
	Static,
	Bone
};

/**
 * 
 */
UCLASS(Abstract)
class GMCEXTENDEDANIMATION_API UGMCE_RootMotionModifier_Warp : public UGMCE_RootMotionModifier
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ExposeOnSpawn))
	FName WarpTargetName { NAME_None };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	EGMCE_MotionWarpProvider WarpPointAnimProvider { EGMCE_MotionWarpProvider::None };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "WarpPointAnimProvider == EGMCE_MotionWarpProvider::Static", EditConditionHides))
	FTransform WarpPointAnimTransform { FTransform::Identity };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "WarpPointAnimProvider == EGMCE_MotionWarpProvider::Bone", EditConditionHides))
	FName WarpPointAnimBoneName { NAME_None };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bWarpTranslation { true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bWarpTranslation", EditConditionHides))
	bool bIgnoreZAxis { true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bWarpTranslation", EditConditionHides))
	EAlphaBlendOption AddTranslationEasingFunc { EAlphaBlendOption::Linear };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (EditCondition = "AddTranslationEasingFunc==EAlphaBlendOption::Custom", EditConditionHides))
	TObjectPtr<class UCurveFloat> AddTranslationEasingCurve { nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bWarpRotation { true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bWarpRotation", EditConditionHides))
	EGMCE_MotionWarpRotationType RotationType { EGMCE_MotionWarpRotationType::Default };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bWarpRotation", EditConditionHides))
	EGMCE_MotionWarpRotationMethod RotationMethod { EGMCE_MotionWarpRotationMethod::Slerp };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "RotationMethod!=EGMCE_MotionWarpRotationMethod::ConstantRate && bWarpRotation", EditConditionHides))
	float WarpRotationTimeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "RotationMethod!=EGMCE_MotionWarpRotationMethod::Slerp && bWarpRotation", EditConditionHides))
	float WarpMaxRotationRate = 0.f;

	UGMCE_RootMotionModifier_Warp(const FObjectInitializer& ObjectInitializer);

	virtual void Update(const FGMCE_MotionWarpContext& Context) override;

	virtual void OnTargetTransformChanged();

	FORCEINLINE FVector GetTargetLocation() const { return CachedTargetTransform.GetLocation(); }
	FORCEINLINE FRotator GetTargetRotator(const FGMCE_MotionWarpContext& WarpContext) const { return GetTargetRotation(WarpContext).Rotator(); }
	FQuat GetTargetRotation(const FGMCE_MotionWarpContext& WarpContext) const;

	FQuat WarpRotation(const FGMCE_MotionWarpContext& WarpContext, const FTransform& RootMotionDelta, const FTransform& RootMotionTotal, float DeltaSeconds);

	FString DisplayString() const override;
	
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	void PrintLog(const FString& Name, const FTransform& OriginalRootMotion, const FTransform& WarpedRootMotion) const;
#endif
	
protected:

	UPROPERTY()
	FTransform CachedTargetTransform { FTransform::Identity };

	TOptional<FTransform> CachedOffsetFromWarpPoint;
	
};
