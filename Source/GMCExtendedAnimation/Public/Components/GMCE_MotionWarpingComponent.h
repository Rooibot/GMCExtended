#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_GMCExMotionWarp.h"
#include "GMCE_MotionWarpSubject.h"
#include "GMCE_MotionWarpTarget.h"
#include "GMCE_RootMotionModifier.h"
#include "GMCE_SharedVariableComponent.h"
#include "Components/ActorComponent.h"
#include "Containers/Map.h"
#include "GMCE_MotionWarpingComponent.generated.h"


class AGMC_Pawn;
class UGMCE_OrganicMovementCmp;

USTRUCT(BlueprintType)
struct FGMCE_MotionWarpingWindowData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	TObjectPtr<UAnimNotifyState_GMCExMotionWarp> AnimNotify = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	float EndTime = 0.f;
};

/**
 * A container structure for motion warping target records. This exists both so that it can be set up to handle
 * updates via delta (instead of sending the entire collection of targets each time there's an update), and also
 * so that it can be wrapped as an FInstancedStruct to be bound via GMC.
 */
USTRUCT()
struct FGMCE_MotionWarpTargetContainer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FGMCE_MotionWarpTarget>	WarpTargets;

	bool FindAndUpdateTarget(FGMCE_MotionWarpTarget& Target)
	{
		for (int32 Idx = 0; Idx < WarpTargets.Num(); Idx++)
		{
			if (WarpTargets[Idx].Name == Target.Name)
			{
				if (WarpTargets[Idx] != Target)
				{
					// Only update and mark dirty if we aren't just setting an identical value.
					WarpTargets[Idx] = Target;
				}
				return true;
			}
		}

		return false;
	}
	
	void AddOrUpdateTarget(FGMCE_MotionWarpTarget& Target)
	{
		if (!FindAndUpdateTarget(Target))
		{
			WarpTargets.Add(Target);
		}	
	}
	
	void RemoveTargetByName(FName TargetName)
	{
		const int32 NumRemoved = WarpTargets.RemoveAll([&TargetName](const FGMCE_MotionWarpTarget& WarpTarget) { return WarpTarget.Name == TargetName; });
		
	}

	void RemoveAllTargets()
	{
		WarpTargets.Empty();
	}

	TArray<FGMCE_MotionWarpTarget> GetTargets() const { return WarpTargets; }

	FString ToString() const
	{
		FString Result = FString(TEXT("{ "));
		bool bFirst = true;

		for (const auto& Target : WarpTargets)
		{
			if (!bFirst)
			{
				Result.Append(TEXT(", "));
			}
			bFirst = false;
			Result += FString::Printf(TEXT("( %s )"), *Target.ToString());
		}

		Result.Append(TEXT(" }"));
		return Result;
	}

};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGMCExPreMotionWarpingDelegate, class UGMCE_MotionWarpingComponent*, MotionWarpingComp);

UCLASS(ClassGroup=(GMCExtended), meta=(BlueprintSpawnableComponent, DisplayName="GMCExtended Motion Warping Component"))
class GMCEXTENDEDANIMATION_API UGMCE_MotionWarpingComponent : public UActorComponent, public IGMCE_SharedVariableComponent
{
	GENERATED_BODY()

public:
	// configuration

	/// If true, we will search for modifier windows in the individual animation sequences within a montage.
	/// If false, we will only look for modifiers in the montage itself. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bSearchForWindowsInAnims { false };

	UPROPERTY(BlueprintAssignable, Category="Motion Warping")
	FGMCExPreMotionWarpingDelegate OnPreUpdate;

	// Sets default values for this component's properties
	UGMCE_MotionWarpingComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UGMCE_OrganicMovementCmp* GetMovementComponent() const { return MovementComponent; }
	AGMC_Pawn* GetOwningPawn() const { return OwningPawn; }

	// We aren't actually using GMCEx's shared variable functionality, but we ARE hijacking this
	// function so that we'll be called when variables are being bound for replication, even if
	// the components were added via Blueprint.
	virtual void OnBindSharedVariables_Implementation(UGMCE_CoreComponent* BaseComponent) override;

	/**
	 * Adds a new motion warp target, or updates an existing one if the name matches. It is recommended you call
	 * this on the server, to avoid shenanigans with someone warping a montage in such a way to let themselves warp
	 * across a map. Calling it on the server is not *enforced*, however if you want to call it from the client
	 * then validating that the motion warping targets are realistic is left as a game-specific exercise. Regardless,
	 * server values will be replicated to the client.
	 * @param Target The motion warp target to add.
	 */
	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void AddOrUpdateWarpTarget(UPARAM(ref) FGMCE_MotionWarpTarget& Target);

	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void AddOrUpdateWarpTargetFromTransform(FName WarpTargetName, FTransform TargetTransform);

	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void AddOrUpdateWarpTargetFromComponent(FName WarpTargetName, USceneComponent* Component, FName BoneName, bool bFollowComponent);

	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void AddOrUpdateWarpTargetFromLocation(FName WarpTargetName, FVector Location);

	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void AddOrUpdateWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector Location, FRotator Rotation);

	FORCEINLINE const FGMCE_MotionWarpTarget* FindWarpTarget(const FName& WarpTargetName) const 
	{ 
		return WarpTargetContainerInstance.Get<FGMCE_MotionWarpTargetContainer>().GetTargets().FindByPredicate([&WarpTargetName](const FGMCE_MotionWarpTarget& WarpTarget){ return WarpTarget.Name == WarpTargetName; });
	}	
	
	/**
	 * Removes a warp target, if a matching one exists. When called on server, will replicate the operation to the client.
	 * @param Target The motion warp target to remove (or at least, a target containing the correct name).
	 */
	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void RemoveWarpTarget(UPARAM(ref) FGMCE_MotionWarpTarget& Target);

	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void RemoveWarpTargetByName(FName Name);

	/**
	 * Removes all current motion warp targets. When called on the server, will replicate the operation to the client.
	 */
	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void RemoveAllWarpTargets();

	FORCEINLINE const TArray<UGMCE_RootMotionModifier*>& GetModifiers() const { return Modifiers; }

	bool ContainsModifier(const UAnimSequenceBase* Animation, float StartTime, float EndTime) const;

	int32 AddModifier(UGMCE_RootMotionModifier* Modifier);

	UFUNCTION(BlueprintCallable, Category="GMC Extended|Motion Warping")
	void DisableAllRootMotionModifiers();

	UGMCE_RootMotionModifier* AddModifierFromTemplate(UGMCE_RootMotionModifier* Template, const UAnimSequenceBase* Animation, float StartTime, float EndTime);

	const FGMCE_MotionWarpTargetContainer& GetWarpTargets() const { return WarpTargetContainerInstance.Get<FGMCE_MotionWarpTargetContainer>(); }
	
	void BindToMovementComponent();

protected:

	// We use BeginPlay rather than InitializeComponent so that we know we can pick up components if they were
	// added in blueprints.
	virtual void BeginPlay() override;

	virtual void OnSyncDataApplied(const FGMC_PawnState& State, EGMC_NetContext Context);

	// Called by GMCEx when processing root motion montages; this is where the meat of the
	// motion warping happens, as it is what actually modifies the transform before GMCv2
	// applies it.
	virtual FTransform ProcessRootMotion(const FTransform& InTransform, UGMCE_OrganicMovementCmp* MovementComponent, float DeltaSeconds);
	
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, AdvancedDisplay, Category="Motion Warping")
	UGMCE_OrganicMovementCmp* MovementComponent;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, AdvancedDisplay, Category="Motion Warping")
	AGMC_Pawn* OwningPawn;
	
	class IGMCE_MotionWarpSubject* MotionWarpSubject;

	UPROPERTY(Transient)
	FInstancedStruct WarpTargetContainerInstance;

	int BI_TargetUpdateBinding { -1 };

	// Currently active root motion modifiers.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGMCE_RootMotionModifier>> Modifiers;

	UPROPERTY()
	UAnimInstance* AnimInstance;

	void Update(float DeltaSeconds);
	
private:
	void AddOrUpdateWarpTarget_Internal(FGMCE_MotionWarpTarget& Target);
	void RemoveWarpTarget_Internal(FName TargetName);
	void RemoveAllWarpTargets_Internal();
	
};
