#pragma once

#include "CoreMinimal.h"
#include "GMCE_MotionWarpTarget.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct GMCEXTENDEDANIMATION_API FGMCE_MotionWarpTarget
{
    GENERATED_BODY()

	/** Unique name for this warp target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FName Name;

	/** When the warp target is created from a component this stores the location of the component at the time of creation, otherwise its the location supplied by the user */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FVector Location;

	/** When the warp target is created from a component this stores the rotation of the component at the time of creation, otherwise its the rotation supplied by the user */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FRotator Rotation;

	/** Optional component used to calculate the final target transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	TWeakObjectPtr<const USceneComponent> Component;

	/** Optional bone name in the component used to calculate the final target transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FName BoneName;

	/** Whether the target transform calculated from a component and an optional bone should be updated during the warp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	bool bFollowComponent;

	FGMCE_MotionWarpTarget()
		: Name(NAME_None), Location(FVector::ZeroVector), Rotation(FRotator::ZeroRotator), Component(nullptr), BoneName(NAME_None), bFollowComponent(false) {}
	
	FGMCE_MotionWarpTarget(const FName& InName, const FTransform& InTransform)
		: Name(InName), Location(InTransform.GetLocation()), Rotation(InTransform.Rotator()), Component(nullptr), BoneName(NAME_None), bFollowComponent(false) {}

	FGMCE_MotionWarpTarget(const FName& InName, const USceneComponent* InComp, FName InBoneName, bool bInbFollowComponent);

	FTransform GetTargetTransform() const;

	FTransform GetTargetTransformFromAnimation(const FTransform& Origin, const FTransform& ComponentRelative, const UAnimInstance* AnimInstance, const UAnimSequenceBase* Animation, float Timestamp) const;

	FORCEINLINE FVector GetLocation() const { return GetTargetTransform().GetLocation(); }
	FORCEINLINE FQuat GetRotation() const { return GetTargetTransform().GetRotation(); }
	FORCEINLINE FRotator Rotator() const { return GetTargetTransform().Rotator(); }

	FORCEINLINE bool operator==(const FGMCE_MotionWarpTarget& Other) const
	{
		return Other.Name == Name && Other.Location.Equals(Location) && Other.Rotation.Equals(Rotation) && Other.Component == Component && Other.BoneName == BoneName && Other.bFollowComponent == bFollowComponent;
	}

	FORCEINLINE bool operator!=(const FGMCE_MotionWarpTarget& Other) const
	{
		return Other.Name != Name || !Other.Location.Equals(Location) || !Other.Rotation.Equals(Rotation) || Other.Component != Component || Other.BoneName != BoneName || Other.bFollowComponent != bFollowComponent;
	}

	static FTransform GetTargetTransformFromComponent(const USceneComponent* Comp, const FName& BoneName);

	static FTransform GetTargetTransformFromAnimation(const UAnimInstance* AnimInstance, const UAnimSequenceBase* Animation, float Timestamp, const FName& BoneName, const FTransform& ComponentToWorld);

	FString ToString() const
	{
		return FString::Printf(TEXT("[%s] %s"), *Name.ToString(), *GetTargetTransform().ToString());
	}
};