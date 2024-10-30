#pragma once

#include "CoreMinimal.h"
#include "GMCE_RootMotionModifier.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_GMCExMotionWarp.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName="GMCEx Motion Warping"))
class GMCEXTENDEDANIMATION_API UAnimNotifyState_GMCExMotionWarp : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category="Settings")
	TObjectPtr<UGMCE_RootMotionModifier> RootMotionModifier;

	UAnimNotifyState_GMCExMotionWarp(const FObjectInitializer& ObjectInitializer);

	void OnBecomeRelevant(UGMCE_MotionWarpingComponent* Component, const UAnimSequenceBase* Animation, float StartTime, float EndTime) const;

	UFUNCTION(BlueprintNativeEvent, Category="Motion Warping")
	UGMCE_RootMotionModifier* AddRootMotionModifier(UGMCE_MotionWarpingComponent* Component, const UAnimSequenceBase* Animation, float StartTime, float EndTime) const;

	UFUNCTION()
	void OnRootMotionModifierActivate(UGMCE_MotionWarpingComponent* Component, UGMCE_RootMotionModifier* Modifier);

	UFUNCTION()
	void OnRootMotionModifierUpdate(UGMCE_MotionWarpingComponent* Component, UGMCE_RootMotionModifier* Modifier);

	UFUNCTION()
	void OnRootMotionModifierDeactivate(UGMCE_MotionWarpingComponent* Component, UGMCE_RootMotionModifier* Modifier);

	UFUNCTION(BlueprintImplementableEvent, Category="Motion Warping")
	void OnWarpBegin(UGMCE_MotionWarpingComponent* Component, UGMCE_RootMotionModifier* Modifier) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Motion Warping")
	void OnWarpUpdate(UGMCE_MotionWarpingComponent* Component, UGMCE_RootMotionModifier* Modifier) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Motion Warping")
	void OnWarpEnd(UGMCE_MotionWarpingComponent* Component, UGMCE_RootMotionModifier* Modifier) const;

#if WITH_EDITOR
	virtual FString GetNotifyName_Implementation() const override;
	virtual void ValidateAssociatedAssets() override;
#endif
};
