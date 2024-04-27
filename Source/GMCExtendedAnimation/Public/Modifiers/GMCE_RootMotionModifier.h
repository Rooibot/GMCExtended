#pragma once

#include "CoreMinimal.h"
#include "GMCPawn.h"
#include "UObject/Object.h"
#include "GMCE_RootMotionModifier.generated.h"

class UGMCE_MotionWarpingComponent;
class UGMCE_RootMotionModifier;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGMCExRootMotionModifierDelegate, UGMCE_MotionWarpingComponent*, MotionComponent, UGMCE_RootMotionModifier*, Modifier);

USTRUCT()
struct FGMCE_MotionWarpContext
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

	
};

UENUM(BlueprintType)
enum class EGMCE_RootMotionModifierState : uint8
{
	Waiting,
	Active,
	MarkedForRemoval,
	Disabled
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnGMCExRootMotionModifierDelegate, UGMCE_MotionWarpingComponent*, MotionWarpingComp, UGMCE_RootMotionModifier*, RootMotionModifier);

/**
 * 
 */
UCLASS(BlueprintType, Abstract, EditInlineNew)
class GMCEXTENDEDANIMATION_API UGMCE_RootMotionModifier : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category="Defaults")
	TWeakObjectPtr<const UAnimSequenceBase> AnimationSequence { nullptr };

	UPROPERTY(BlueprintReadOnly, Category="Defaults")
	float StartTime { 0.f };

	UPROPERTY(BlueprintReadOnly, Category="Defaults")
	float EndTime { 0.f };

	UPROPERTY(BlueprintReadOnly, Category="Defaults")
	float PreviousPosition { 0.f };

	UPROPERTY(BlueprintReadOnly, Category="Defaults")
	float CurrentPosition { 0.f };

	UPROPERTY(BlueprintReadOnly, Category="Defaults")
	float Weight { 0.f };

	UPROPERTY(BlueprintReadOnly, Transient, Category="Defaults")
	FTransform StartTransform { FTransform::Identity };

	UPROPERTY(BlueprintReadOnly, Transient, Category="Defaults")
	float ActualStartTime { 0.f };

	UPROPERTY()
	FOnGMCExRootMotionModifierDelegate OnActivateDelegate;

	UPROPERTY()
	FOnGMCExRootMotionModifierDelegate OnUpdateDelegate;

	UPROPERTY()
	FOnGMCExRootMotionModifierDelegate OnDeactivateDelegate;

	UGMCE_RootMotionModifier(const FObjectInitializer& ObjectInitializer);

	virtual void OnStateChanged(EGMCE_RootMotionModifierState PreviousState);

	void SetState(EGMCE_RootMotionModifierState NewState);

	FORCEINLINE EGMCE_RootMotionModifierState GetState() const { return State; }

	UGMCE_MotionWarpingComponent* GetOwnerComponent() const;

	AGMC_Pawn* GetPawnOwner() const;

	virtual void Update(const FGMCE_MotionWarpContext& Context);
	virtual FTransform ProcessRootMotion(const FTransform& InRootMotion, float DeltaSeconds) { return FTransform::Identity; }

	FORCEINLINE const UAnimSequenceBase* GetAnimation() const { return AnimationSequence.Get(); }

	FString ToString() const;
	
private:

	friend UGMCE_MotionWarpingComponent;
	
	EGMCE_RootMotionModifierState State;
};
