#pragma once

#include "CoreMinimal.h"
#include "GMCE_CoreComponent.h"
#include "GMCOrganicMovementComponent.h"
#include "Containers/RingBuffer.h"
#include "Solvers/GMCE_BaseSolver.h"
#include "Support/GMCEMovementSample.h"
#include "GMCE_OrganicMovementCmp.generated.h"

DECLARE_DELEGATE_RetVal_ThreeParams(FTransform, FOnProcessRootMotionEx, const FTransform&, UGMCE_OrganicMovementCmp*, float)
DECLARE_DELEGATE_TwoParams(FOnSyncDataApplied, const FGMC_PawnState&, EGMC_NetContext)
DECLARE_DELEGATE(FOnBindReplicationData)

DECLARE_LOG_CATEGORY_EXTERN(LogGMCExtended, Log, All)

class UGMCE_BaseSolver;

UCLASS(ClassGroup=(GMCExtended), meta=(BlueprintSpawnableComponent, DisplayName="GMCExtended Organic Movement Component"))
class GMCEXTENDED_API UGMCE_OrganicMovementCmp : public UGMCE_CoreComponent
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
	virtual FVector PreProcessInputVector_Implementation(FVector InRawInputVector) override;
	virtual void PreMovementUpdate_Implementation(float DeltaSeconds) override;
	virtual void MovementUpdate_Implementation(float DeltaSeconds) override;
	virtual void GenSimulationTick_Implementation(float DeltaTime) override;
	virtual void GenAncillaryTick_Implementation(float DeltaTime, bool bLocalMove, bool bCombinedClientMove) override;
	virtual bool UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds) override;
	virtual void OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode) override;
	virtual void OnMovementModeChangedSimulated_Implementation(EGMC_MovementMode PreviousMovementMode) override;

	virtual void PhysicsCustom_Implementation(float DeltaSeconds) override;
	virtual float GetInputAccelerationCustom_Implementation() const override;
	virtual void CalculateVelocity(float DeltaSeconds) override;

	virtual void RotateYawTowardsDirection(const FVector& Direction, float Rate, float DeltaTime) override;
	virtual bool RotateYawTowardsDirectionSafe(const FVector& Direction, float Rate, float DeltaTime) override;

	virtual UPrimitiveComponent* FindActorBase_Implementation() override;

	virtual void ApplyRotation(bool bIsDirectBotMove, const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) override;

	// General functionality

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
	/// When true, the pawn will smoothly rotate around the yaw axis to face the current direction of movement.
	/// This setting will take precedence over bOrientToInputDirection or bOrientToControlRotationDirection if
	/// either of the other two are set. This is very similar to bOrientToInputDirection, but driven by the actual
	/// movement rather than input. If both OrientToInputDirection and bOrientToVelocityDirection are set,
	/// Input Direction will be utilized when starting from a standstill.
	bool bOrientToVelocityDirection{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
	/// If both bOrientToVelocityDirection and bOrientToInputDirection are true, the ground speed must be
	/// greater than this amount to orient towards the velocity; any lower and it will orient towards input instead.
	float MinimumVelocityForOrientation { 100.f };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Operation")
	/// If true, if the direction a character is trying to move differs from the current forward vector
	/// by more than a certain amount, the character will only rotate rather than actually moving. This should
	/// only be used with non-strafing movement, and generally in conjunction with either bOrientToInputDirection
	/// or the combined bOrientToInputDirection / bOrientToVelocityDirection mode.
	bool bRequireFacingBeforeMove{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Operation")
	/// When bRequireFacingBeforeMove is true, if the character attempts to move in a direction more than
	/// this many degrees off from the current 'forward' direction, they will rotate until they are within
	/// this threshold of their movement direction before they begin moving forward.
	float FacingAngleOffsetThreshold { 25.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Operation")
	/// If this is a non-zero positive number and bOrientToControlRotationDirection is true, we will wait
	/// this many seconds before we turn-in-place when velocity is zero.
	float TurnInPlaceDelay { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Operation")
	/// If this is a non-zero positive number and bOrientToControlRotationDirection is true, we will wait
	/// until this angle is reached from the pawn's rotation before turning towards the control rotation
	float TurnInPlaceAngleThreshold { 80.f };

	float TurnInPlaceSecondsAccumulated { 0.f };
	FVector TurnInPlaceDelayedDirection { 0.f };
	
	FVector TurnToDirection { 0.f };

	UFUNCTION(BlueprintCallable, Category="Movement")
	void SetStrafingMovement(bool bStrafingEnabled = false);

	bool ShouldTurnInPlace(const FGMC_RootMotionVelocitySettings& RootMotionMetaData);
	bool ShouldContinueTurnInPlace(const float angle);
	void HandleTurnInPlace(float DeltaSeconds);
	
	// Utilities

	// Debug
	
	/// If called in a context where the DrawDebug calls are disabled, this function will do nothing.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void EnableTrajectoryDebug(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	bool IsTrajectoryDebugEnabled() const;

#pragma region Animation Support
protected:
	virtual void MontageUpdate(float DeltaSeconds) override;
	virtual void OnSyncDataApplied_Implementation(const FGMC_PawnState& State, EGMC_NetContext Context) override;

	void UpdateAnimationHelperValues(float DeltaSeconds);
	void CalculateAimYawRemaining(const FVector& DirectionVector);
	
public:
	FRotator GetCurrentAimRotation() const { return CurrentAimRotation; }
	FRotator GetLastAimRotation() const { return LastAimRotation; }
	float GetCurrentAimYawRate() const { return CurrentAimYawRate; }

	FRotator GetCurrentComponentRotation() const { return CurrentComponentRotation; }
	FRotator GetLastComponentRotation() const { return LastComponentRotation; }
	float GetCurrentComponentYawRate() const { return CurrentComponentYawRate; }

	FVector GetCurrentAnimationAcceleration() const { return CurrentAnimationAcceleration; }

	float GetAimYawRemaining() const { return AimYawRemaining; }
	
	FOnProcessRootMotionEx ProcessRootMotionPreConvertToWorld;
	FOnSyncDataApplied OnSyncDataAppliedDelegate;
	FOnBindReplicationData OnBindReplicationData;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	FRotator CurrentAimRotation { FRotator::ZeroRotator };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	FRotator LastAimRotation { FRotator::ZeroRotator };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	float CurrentAimYawRate { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	float AimYawRemaining { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	FRotator CurrentComponentRotation { FRotator::ZeroRotator };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	FRotator LastComponentRotation { FRotator::ZeroRotator };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	float CurrentComponentYawRate { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	FVector CurrentAnimationAcceleration { 0.f };

	FVector LastAnimationVelocity { 0.f };
	
#pragma endregion
	
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
	void UpdateStopPrediction(float DeltaTime);

	/// Calls the pivot point prediction logic; the result will be cached in the PredictedPivotPoint and
	/// TrajectoryIsPivoting properties.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void UpdatePivotPrediction(float DeltaTime);

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

	/// The angle of difference which we must exceed before a pivot can be predicted. Must be
	/// between 90 and 179. Recommended values are between 90 and 135.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory", meta=(UIMin="90", UIMax="179"))
	float PivotPredictionAngleThreshold { 90.f };
	
	UFUNCTION(BlueprintPure, Category="Trajectory Matching", meta=(ToolTip="Returns a predicted point relative to the actor where they'll come to a stop.", BlueprintThreadSafe))
	static FVector PredictGroundedStopLocation(const FVector& CurrentVelocity, float BrakingDeceleration, float Friction, float DeltaTime);

	UFUNCTION(BlueprintPure, Category="Trajectory Matching", meta=(ToolTip="Returns a predicted point relative to the actor where they'll finish a pivot.", BlueprintThreadSafe))
	static FVector PredictGroundedPivotLocation(const FVector& CurrentAcceleration, const FVector& CurrentVelocity, const FRotator& CurrentRotation, float Friction, float DeltaTime, float AngleThreshold = 90.f);


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

	/// Should we start with trajectory debug enabled? Only valid in editor.
	UPROPERTY(EditDefaultsOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	bool bDrawDebugPredictions { false };
	
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

	/// Return the current set of historical trajectory samples. Coordinates are relative to current location and
	/// rotation.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	FGMCE_MovementSampleCollection GetMovementHistory(bool bOmitLatest) const;

	/// Given the historical samples, predict the future trajectory a character will take. Coordinates are relative
	/// to the origin point provided.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	FGMCE_MovementSampleCollection PredictMovementFuture(const FTransform& FromOrigin, bool bIncludeHistory) const;

	/// Update the cached trajectory prediction. Called automatically if trajectory precalculation is enabled.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	virtual void UpdateTrajectoryPrediction();
	
	/// Should historical trajectory samples be taken?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory")
	bool bTrajectoryEnabled { true };

	/// Should the component automatically calculate a predicted trajectory and have it always available?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory|Precalculations")
	bool bPrecalculateFutureTrajectory { true };

	/// The maximum size of the trajectory sample history.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	int32 MaxTrajectorySamples = { 200 };

	/// The maximum time-window of trajectory samples that should be kept.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	float TrajectoryHistorySeconds { 2.f };

	/// How many simulated samples should be generated for each second, when predicting trajectory?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	int32 TrajectorySimSampleRate = { 30 };

	/// How many seconds of prediction should we generate when predicting trajectory?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	float TrajectorySimSeconds = { 1.f };

	/// The last predicted trajectory. Only valid if PrecalculateFutureTrajectory is true, or
	/// UpdateTrajectoryPrediction has been manually called.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement Trajectory")
	FGMCE_MovementSampleCollection PredictedTrajectory;
	
protected:

	/// Obtain a movement sample representing our current pawn state.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	FGMCE_MovementSample GetMovementSampleFromCurrentState() const;

	/// Add a new movement sample to the history, accumulating time and transform to
	/// all previous samples.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	void AddNewMovementSample(const FGMCE_MovementSample& Sample);

	/// Clear out old samples from the trajectory sample history.
	void CullMovementSampleHistory(bool bIsNearlyZero, const FGMCE_MovementSample& LatestSample);

	/// When trajectory is enabled, this is called to generate a new sample and add it to the history.
	/// The default version generates a sample, adds it, and then calls Update Trajectory Prediction if
	/// trajectory precalculation is enabled.
	UFUNCTION(BlueprintNativeEvent, Category="Movement Trajectory")
	void UpdateMovementSamples();

	/// Get our current acceleration and rotational velocity from our historical movement samples.
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
	/// Enables ragdolling. The character's actual position will be maintained (for the sake of everything staying
	/// in sync), but velocity will be preserved as the skeletal mesh is ragdolled.
	UFUNCTION(BlueprintCallable, Category="Ragdoll")
	void EnableRagdoll();

	/// Disables ragdolling. The skeletal mesh will snap back to the actual character position, and resume grounded
	/// movement.
	UFUNCTION(BlueprintCallable, Category="Ragdoll")
	void DisableRagdoll();

	/// Has the ragdoll state been enabled?
	UFUNCTION(BlueprintPure, Category="Ragdoll")
	bool RagdollActive() const { return bWantsRagdoll; }

	virtual EGMC_MovementMode GetRagdollMode() const { return EGMC_MovementMode::Custom1; }

	/// Can be overridden to provide a custom value for the initial linear velocity for a ragdoll. Will be called
	/// on the owning client. Default implementation just returns the value of `GetLinearVelocity_GMC`.
	UFUNCTION(BlueprintNativeEvent, Category="Ragdoll")
	FVector GetRagdollInitialVelocity();

	/// If true, will ensure ragdolls land in the same place on all clients and that the character is moved there.
	/// If false, ragdolls will be entirely local. Should be true if you intend to have the character get up afterwards,
	/// false if you want to just use ragdoll animations on death and will be respawning afterwards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ragdoll")
	bool bShouldReplicateRagdoll { true };

	/// The bone to use for ragdolling.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ragdoll")
	FName RagdollBoneName = FName(TEXT("pelvis"));	
	
private:

	void SetRagdollActive(bool bActive);

	int32 BI_ShouldReplicateRagdoll { -1 };
	int32 BI_RagdollBoneName { -1 };

	FVector CurrentRagdollGoal { 0.f };
	int32 BI_CurrentRagdollGoal { -1 };
	
	bool bWantsRagdoll { false };
	int32 BI_WantsRagdoll { -1 };

	bool bResetMesh { false };
	bool bFirstRagdollTick { false };

	FVector RagdollLinearVelocity { 0.f };
	int32 BI_RagdollLinearVelocity { -1 };

	FVector PreviousRelativeMeshLocation { 0.f };
	FRotator PreviousRelativeMeshRotation { 0.f };
	float PreviousCollisionHalfHeight { 0.f };
	
#pragma endregion

	// Solvers
#pragma region Solvers
public:

	UFUNCTION(BlueprintCallable, Category="Parcore|Solvers")
	void RunSolvers(float DeltaTime);

	bool ShouldDebugSolver(const FGameplayTag& SolverTag) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Parcore|Solvers")
	bool IsSolutionAvailableForSolver(FGameplayTag SolverTag) const;

	UFUNCTION(BlueprintCallable, Category="Parcore|Solvers")
	UGMCE_BaseSolver* GetActiveSolver();

	UFUNCTION(BlueprintCallable, Category="Parcore|Solvers")
	UGMCE_BaseSolver* GetSolverForTag(FGameplayTag SolverTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Parcore|Solvers")
	FGameplayTag GetActiveSolverTag() const { return CurrentActiveSolverTag; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Parcore|Solvers")
	virtual EGMC_MovementMode GetSolverMovementMode() const { return EGMC_MovementMode::Custom2; }

	UFUNCTION(BlueprintCallable, Category="Parcore|Solvers")
	bool TryActivateSolver(const FGameplayTag& SolverTag);

	FSolverState GetSolverState() const;

	virtual void OnSolverChangedMode(FGameplayTag NewMode, FGameplayTag OldMode) {};

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Solvers")
	TArray<TSubclassOf<UGMCE_BaseSolver>> SolverClasses {};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category="Solvers")
	TArray<TObjectPtr<UGMCE_BaseSolver>> Solvers {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solvers")
	FGameplayTagContainer DebugSolvers;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Solvers")
	FGameplayTagContainer AvailableSolvers;
	int32 BI_AvailableSolvers { -1 };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Solvers")
	FGameplayTag CurrentActiveSolverTag;
	int32 BI_CurrentActiveSolverTag { -1 };

	UPROPERTY()
	TObjectPtr<UGMCE_BaseSolver> CurrentActiveSolver;

	UPROPERTY()
	UPrimitiveComponent* SolverAppliedBase { nullptr };
	
private:

	
	

#pragma endregion
	
};
