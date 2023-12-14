// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GMCOrganicMovementComponent.h"
#include "Containers/RingBuffer.h"
#include "Support/GMCEMovementSample.h"
#include "GMCE_OrganicMovementCmp.generated.h"

static EGMC_MovementMode MovementRagdoll = EGMC_MovementMode::Custom1;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="GMCExtended Organic Movement Component"))
class GMCEXTENDED_API UGMCE_OrganicMovementCmp : public UGMC_OrganicMovementCmp
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGMCE_OrganicMovementCmp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// GMC Overrides
	virtual void BindReplicationData_Implementation() override;
	virtual void MovementUpdate_Implementation(float DeltaSeconds) override;
	virtual void GenSimulationTick_Implementation(float DeltaTime) override;
	virtual bool UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds) override;
	virtual void OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode) override;
	
	// Utilities

	// Debug
	
	/// If called in a context where the DrawDebug calls are disabled, this function will do nothing.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void EnableTrajectoryDebug(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	bool IsTrajectoryDebugEnabled() const;
	
	// Trajectory state functionality (input presence, acceleration synthesis for simulated proxies, etc.)
#pragma region Trajectory State
public:

	/// If true, this pawn is actively being provided with an input acceleration.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	bool IsInputPresent(bool bAllowGrace = false) const;

	/// If true, velocity differs enough from input acceleration that we're likely to pivot.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	bool DoInputAndVelocityDiffer() const { return FMath::Abs(InputVelocityOffset) > 90.f; }

	/// The angle, in degrees, that velocity differs from provided input (on the XY plane).
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	float InputVelocityOffsetAngle() const { return InputVelocityOffset; }

	/// The current effective acceleration we're moving at. This differs from input acceleration, as it's
	/// calculated from movement history.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	FVector GetCurrentEffectiveAcceleration() const { return CalculatedEffectiveAcceleration; }

private:

	void UpdateCalculatedEffectiveAcceleration();
	
	// Input state, for stop detection
	bool bInputPresent { false };
	int32 BI_InputPresent { -1 };

	// The angle, in degrees, by which our input acceleration and current linear velocity differ on the XY plane.
	float InputVelocityOffset { 0.f };
	int32 BI_InputVelocityOffset { -1 };

	// Current effective acceleration, for pivot calculation
	FVector CalculatedEffectiveAcceleration { 0.f };

#pragma endregion	

	// Stop/pivot point prediction, for distance matching animation.
#pragma region Stop/Pivot Prediction
public:
	/// Calls the stop point prediction logic; the result will be cached in the PredictedStopPoint and
	/// TrajectoryIsStopping properties.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void UpdateStopPrediction();

	/// Calls the pivot point prediction logic; the result will be cached in the PredictedPivotPoint and
	/// TrajectoryIsPivoting properties.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void UpdatePivotPrediction();

	/// Check whether a stop is predicted, and store the prediction in OutStopPrediction. Only valid
	/// if UpdateStopPrediction has been called, or PrecalculateDistanceMatches is true.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	bool IsStopPredicted(FVector &OutStopPrediction) const;

	/// Check whether a pivot is predicted, and store the prediction in OutPivotPrediction. Only valid
	/// if UpdatePivotPrediction has been called, or PrecalculateDistanceMatches is true.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	bool IsPivotPredicted(FVector &OutPivotPrediction) const;

	/// If true, this component will pre-calculate stop and pivot predictions, so that they can be easily accessed
	/// in a thread-safe manner without needing to manually call the calculations each time.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory|Precalculations")
	bool bPrecalculateDistanceMatches { true };

	UFUNCTION(BlueprintPure, Category="RooiCore Trajectory Matching", meta=(ToolTip="Returns a predicted point relative to the actor where they'll come to a stop.", BlueprintThreadSafe))
	static FVector PredictGroundedStopLocation(const FVector& CurrentVelocity, float BrakingDeceleration, float Friction);

	UFUNCTION(BlueprintPure, Category="RooiCore Trajectory Matching", meta=(ToolTip="Returns a predicted point relative to the actor where they'll finish a pivot.", NotBlueprintThreadSafe))
	static FVector PredictGroundedPivotLocation(const FVector& CurrentAcceleration, const FVector& CurrentVelocity, const FRotator& CurrentRotation, float Friction);


private:

	/// Used by simulated proxies for a very small 'grace period' on predicting pivots, to prevent false negatives.
	bool bHadInput { false };
	
	/// Used by simulated proxies for a very small 'grace period' on predicting pivots, to prevent false negatives.
	double InputStoppedAt { 0 };
	
	/// Whether or not a stop is imminent. Only valid when UpdateStopPrediction has been called,
	/// or PrecalculateDistanceMatches is true.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	bool bTrajectoryIsStopping { false };

	/// A position relative to our location where we predict a stop. Only valid when UpdateStopPrediction
	/// has been called, or PrecalculateDistanceMatches is true.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	FVector PredictedStopPoint { 0.f };

	/// Whether or not a pivot is imminent. Only valid when UpdatePivotPrediction has been called,
	/// or PrecalculateDistanceMatches is true.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	bool bTrajectoryIsPivoting { false };

	/// A position relative to our location where we predict a pivot. Only valid when UpdatePivotPrediction has
	/// been called, or PrecalculateDistanceMatches is true.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	FVector PredictedPivotPoint { 0.f };

#if WITH_EDITORONLY_DATA
	/// Should we start with trajectory debug enabled? Only valid in editor.
	UPROPERTY(EditDefaultsOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	bool bDrawDebugPredictions { false };
#endif
	
#if ENABLE_DRAW_DEBUG || WITH_EDITORONLY_DATA
	/// For debug rendering
	bool bDebugHadPreviousStop { false };
	FVector DebugPreviousStop { 0.f };
	bool bDebugHadPreviousPivot { false };
	FVector DebugPreviousPivot { 0.f };
#endif
	
#pragma endregion 

	// Trajectory history and prediction
#pragma region Trajectory History
public:

	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	FGMCE_MovementSampleCollection GetMovementHistory(bool bOmitLatest) const;

	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	FGMCE_MovementSampleCollection PredictMovementFuture(const FTransform& FromOrigin, bool bIncludeHistory) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory")
	bool bTrajectoryEnabled { true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory|Precalculations")
	bool bPrecalculateFutureTrajectory { true };
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	int32 MaxTrajectorySamples = { 200 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	float TrajectoryHistorySeconds { 2.f };
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	int32 TrajectorySimSampleRate = { 30 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	float TrajectorySimSeconds = { 1.f };

	/// The last predicted trajectory. Only valid if PrecalculateFutureTrajectory is true, or
	/// UpdateTrajectoryPrediction has been manually called.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement Trajectory")
	FGMCE_MovementSampleCollection PredictedTrajectory;
	
protected:

	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void UpdateTrajectoryPrediction();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	FGMCE_MovementSample GetMovementSampleFromCurrentState() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	void AddNewMovementSample(const FGMCE_MovementSample& Sample);

	void CullMovementSampleHistory(bool bIsNearlyZero, const FGMCE_MovementSample& LatestSample);

	UFUNCTION(BlueprintNativeEvent, Category="Movement Trajectory")
	void UpdateMovementSamples();
	
	void GetCurrentAccelerationRotationVelocityFromHistory(FVector& OutAcceleration, FRotator& OutRotationVelocity) const;

	
private:

	TRingBuffer<FGMCE_MovementSample> MovementSamples;
	FGMCE_MovementSample LastMovementSample;
	
	float LastTrajectoryGameSeconds { 0.f };

	float EffectiveTrajectoryTimeDomain { 0.f };	
	
#pragma endregion

	// Ragdoll experiment
#pragma region Ragdoll
public:
	UFUNCTION(BlueprintCallable, Category="Ragdoll")
	void EnableRagdoll();

	UFUNCTION(BlueprintCallable, Category="Ragdoll")
	void DisableRagdoll();

	UFUNCTION(BlueprintPure, Category="Ragdoll")
	bool RagdollActive() const { return bWantsRagdoll; }
	
private:
	
	bool bWantsRagdoll { false };
	int32 BI_WantsRagdoll { -1 };

	// If true, we need to put our skeletal mesh back where we found it on the next component tick.
	bool bResetMesh { false };
	
#pragma endregion
		
};
