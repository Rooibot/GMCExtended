// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GMCE_MotionAnimationComponent.generated.h"


struct FGMCE_MovementSample;
struct FGMCE_MotionWarpTarget;
class UGMCE_OrganicMovementCmp;
class UGMCE_MotionWarpingComponent;

UCLASS(ClassGroup=(GMCExtended), meta=(BlueprintSpawnableComponent, DisplayName="GMCExtended Motion Animation Component"))
class GMCEXTENDEDANIMATION_API UGMCE_MotionAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGMCE_MotionAnimationComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Components")
	UGMCE_MotionWarpingComponent *MotionWarpingComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Components")
	UGMCE_OrganicMovementCmp *OrganicMovementCmp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Motion Animation Component")
	bool bUseBlueprintEvents { true };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Motion Animation Component")
	bool bEnableHandling { false };

	void PlayMontageInternal(UAnimMontage* Montage, float StartPosition, float PlayRate);
	
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void SetEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Motion Animation Component")
	bool IsEnabled() const { return bEnableHandling; }

	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void Reset();

	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void EnableAndPlayMontageFromOriginWithWarpTargets(UAnimMontage* Montage, float StartPosition, float PlayRate, FTransform OriginTransform, FTransform MeshRelativeTransform, TArray<FGMCE_MotionWarpTarget> WarpTargets);

	UFUNCTION(Server, Reliable)
	void SV_EnableAndPlayMontageFromOriginWithWarpTargets(UAnimMontage* Montage, float StartPosition, float PlayRate, FTransform OriginTransform, FTransform MeshRelativeTransform, const TArray<FGMCE_MotionWarpTarget>& WarpTargets);

	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	bool PrecalculatePathFromOriginWithWarpTargets(UAnimMontage* Montage, float StartPosition, float PlayRate, FTransform OriginTransform, FTransform MeshRelativeTransform, TArray<FGMCE_MotionWarpTarget> WarpTargets);
	
	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void SyncToCurrentMontagePosition(float DeltaSeconds, bool bUseMontageTracker = false, float OverrideTime = -1.f);	

	UFUNCTION(BlueprintNativeEvent)
	void PostMovement(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void PostMovementHandler(float DeltaSeconds);

	UFUNCTION(BlueprintNativeEvent)
	void PostMovementSimulated(float DeltaSeconds);
	
	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void PostMovementSimulatedHandler(float DeltaSeconds);
	
	UFUNCTION(BlueprintNativeEvent)
	void PredictionTick(float DeltaSeconds);
	
	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void PredictionTickHandler(float DeltaSeconds);

	UFUNCTION(BlueprintNativeEvent)
	void AncillaryTick(float DeltaSeconds);
	
	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void AncillaryTickHandler(float DeltaSeconds);

	UFUNCTION(BlueprintNativeEvent)
	void SimulationTick(float DeltaSeconds);
	
	UFUNCTION(BlueprintCallable, Category = "Motion Animation Component")
	void SimulationTickHandler(float DeltaSeconds);

private:

	UPROPERTY()
	UAnimMontage* TargetMontage;
	
	float TargetStartPosition { 0.f };
	float TargetPlayRate { 1.f };

	FTransform TargetOriginTransform;
	FTransform TargetMeshRelativeTransform;
	TArray<FGMCE_MotionWarpTarget> TargetWarpTargets;
	
};
