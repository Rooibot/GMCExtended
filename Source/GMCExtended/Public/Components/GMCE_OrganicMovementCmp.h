#pragma once

#include "CoreMinimal.h"
#include "GMCE_CoreComponent.h"
#include "GMCOrganicMovementComponent.h"
#include "Containers/RingBuffer.h"
#include "Solvers/GMCE_BaseSolver.h"
#include "Support/GMCEMovementSample.h"
#include "GMCE_OrganicMovementCmp.generated.h"

// We append GMC to the delegate name because Epic decided to add an FOnProcessRootMotion to the CMC in 5.4.
DECLARE_DELEGATE_RetVal_SixParams(FTransform, FOnProcessRootMotionGMC, const FTransform&, const FTransform&, const FTransform&, UGMCE_OrganicMovementCmp*, float, bool)
DECLARE_DELEGATE_TwoParams(FOnSyncDataApplied, const FGMC_PawnState&, EGMC_NetContext)
DECLARE_DELEGATE(FOnBindReplicationData)

class UGMCE_BaseSolver;

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FGMCE_SpeedMark
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="180", UIMin="0", UIMax="180"), Category="Locomotion Limits")
	float AngleOffset { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1", ClampMax="1.0", UIMin="0.1", UIMax="1.0"), Category="Locomotion Limits")
	float SpeedFactor { 0.f };
};

UENUM(BlueprintType)
enum class EGMCE_TurnInPlaceType : uint8
{
	None,
	MovementComponent,
	TrackedCurveValue
};

UENUM(BlueprintType)
enum class EGMCE_TurnInPlaceState : uint8
{
	Ready,
	Starting,
	Running,
	Done
};

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
	virtual void PreSimulatedMoveExecution_Implementation(FGMC_PawnState& InputState, bool bCumulativeUpdate, bool bRollback, float DeltaTime, double Timestamp) override;
	virtual void MovementUpdate_Implementation(float DeltaSeconds) override;
	virtual void MovementUpdateSimulated_Implementation(float DeltaSeconds) override;
	virtual void GenSimulationTick_Implementation(float DeltaTime) override;
	virtual void GenPredictionTick_Implementation(float DeltaTime) override;
	virtual void GenAncillaryTick_Implementation(float DeltaTime, bool bLocalMove, bool bCombinedClientMove) override;
	virtual bool UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds) override;
	virtual void OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode) override;
	virtual void OnMovementModeChangedSimulated_Implementation(EGMC_MovementMode PreviousMovementMode) override;
	virtual void PostMovementUpdate_Implementation(float DeltaSeconds) override;
	virtual void PostSimulatedMoveExecution_Implementation(const FGMC_PawnState& OutputState, bool bCumulativeUpdate, float DeltaTime, double Timestamp) override;

	virtual float GetMaxSpeed() const override;
	
	virtual void PhysicsCustom_Implementation(float DeltaSeconds) override;
	virtual float GetInputAccelerationCustom_Implementation() const override;
	virtual void CalculateVelocity(float DeltaSeconds) override;

	virtual void ApplyDirectionalInput(const FInputActionInstance& InputAction) override;
	
	virtual void OnLanded_Implementation(const FVector& ImpactVelocity) override;

	virtual void RotateYawTowardsDirection(const FVector& Direction, float Rate, float DeltaTime) override;
	virtual bool RotateYawTowardsDirectionSafe(const FVector& Direction, float Rate,float CollisionTolerance, float DeltaTime) override;

	virtual UPrimitiveComponent* FindActorBase_Implementation() override;

	virtual void ApplyRotation(bool bIsDirectBotMove, const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds) override;

	virtual void MontageUpdate(float DeltaSeconds) override;
	virtual void OnMontageStarted(UAnimMontage* Montage, float Position, float PlayRate, bool bInterrupted, float MontageDelta, float DeltaSeconds) override;

	// General functionality

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GMCExtended|Debug")
	FString GetComponentDescription() const;

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
	/// If true, the angle at which the character is moving will be constrained by the rotation rate as well.
	/// You do not -- ever -- want to set this true in a strafing scenario!
	bool bLockVelocityToRotationRate { false };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Turn-in-Place")
	/// If true, if the direction a character is trying to move differs from the current forward vector
	/// by more than a certain amount, the character will only rotate rather than actually moving. This should
	/// only be used with non-strafing movement, and generally in conjunction with either bOrientToInputDirection
	/// or the combined bOrientToInputDirection / bOrientToVelocityDirection mode.
	bool bRequireFacingBeforeMove{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Turn-in-Place")
	/// When bRequireFacingBeforeMove is true, if the character attempts to move in a direction more than
	/// this many degrees off from the current 'forward' direction, they will rotate until they are within
	/// this threshold of their movement direction before they begin moving forward.
	float FacingAngleOffsetThreshold { 25.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Turn-in-Place")
	/// If TurnInPlaceType is not "None" and bOrientToControllerDirection is true, the character will
	/// remain facing the way they currently appear to be until the controller rotation deviates by at least
	/// this much.
	float TurnInPlaceAngleThreshold { 45.f };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Turn-in-Place")
	/// If turn in place is enabled, this determines how the turn-in-place is handled. 
	EGMCE_TurnInPlaceType TurnInPlaceType { EGMCE_TurnInPlaceType::None };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Turn-in-Place")
	/// If this is a non-zero positive number and bOrientToControlRotationDirection is true, we will wait
	/// this many seconds before we turn-in-place when velocity is zero.
	float TurnInPlaceDelay { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Turn-in-Place")
	/// The rotation rate at which we should turn-in-place. This is separate from normal component
	/// rotation rate because you may want turn-in-place to be a little less zippy.
	float TurnInPlaceRotationRate { 125.f };
	
	float TurnInPlaceSecondsAccumulated { 0.f };
	FVector TurnInPlaceDelayedDirection { 0.f };
	FVector TurnInPlaceStartDirection{ 0.f };
	
	int BI_TurnInPlaceDelayedDirection { -1 };
	int BI_TurnInPlaceStartDirection { -1 };
	
	FVector TurnToDirection { 0.f };
	
	float TurnInPlaceTotalYaw { 0.f };
	float TurnInPlaceLastYaw { 0.f };

	int BI_TurnToDirection{ -1 };

	bool bWantsTurnInPlace { false };
	int BI_WantsTurnInPlace { -1 };
	EGMCE_TurnInPlaceState TurnInPlaceState { EGMCE_TurnInPlaceState::Ready };

	float RootYawOffset { 0.f };
	float RootYawBlendTime { 0.f };

	void UpdateTurnInPlaceState(bool bSimulated = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement")
	bool IsStandalone() const { return GetNetMode() == NM_Standalone; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement")
	bool IsTurningInPlace() const { return TurnInPlaceState == EGMCE_TurnInPlaceState::Running; }

	UFUNCTION(BlueprintCallable, Category="Movement")
	void SetStrafingMovement(bool bStrafingEnabled = false);

	virtual bool ShouldTurnInPlace() const;
	virtual void CalculateTurnInPlace(float DeltaSeconds);
	virtual void ApplyTurnInPlace(float DeltaSeconds, bool bSimulated);
	virtual void EndTurnInPlace(bool bSimulated = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement")
	virtual float GetTurnInPlaceDuration() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Tempo")
	/// If this is true, then if the movement direction differs from the character's forward vector
	/// by more than a certain amount, the maximum speed will be reduced.
	bool bLimitStrafeSpeed { false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement|Tempo", meta=(EditCondition="bLimitStrafeSpeed", EditConditionHides))
	TArray<FGMCE_SpeedMark> StrafeSpeedPoints { { 45.f, 1.f }, { 120.f, 0.5f } };
	
	// Utilities

	static float CalculateDirection(const FVector& Direction, const FRotator& Rotation);
	
	UFUNCTION(BlueprintCallable, Category="Movement")
	float GetLocomotionAngle() const;

	UFUNCTION(BlueprintCallable, Category="Movement")
	float GetOrientationAngle() const;
	
	// Debug
	
	/// If called in a context where the DrawDebug calls are disabled, this function will do nothing.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void EnableTrajectoryDebug(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	bool IsTrajectoryDebugEnabled() const;

private:
	FString ComponentLogDescriptionString {};

#pragma region Animation Support
protected:
	virtual void PreProcessRootMotion(const FGMC_AnimMontageInstance& MontageInstance, FRootMotionMovementParams& InOutRootMotionParams, float MontageDelta, float DeltaSeconds) override;
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
	float GetComponentYawRemaining() const { return ComponentYawRemaining; }

	float GetMaxPredictionSpeed(const FVector& InputVector);
	
	FOnProcessRootMotionGMC ProcessRootMotionPreConvertToWorld;
	FOnSyncDataApplied OnSyncDataAppliedDelegate;
	FOnBindReplicationData OnBindReplicationData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	FVector LastLandingVelocity { 0.f };

	// When a montage is playing, what our previous montage position was. Used for motion warping.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	float PreviousMontagePosition { 0.f };

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
	FRotator TargetComponentRotation { FRotator::ZeroRotator };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	float CurrentComponentYawRate { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	float ComponentYawRemaining { 0.f };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animation Helpers")
	FVector CurrentAnimationAcceleration { 0.f };

	FVector LastAnimationVelocity { 0.f };

	int32 BI_LastLandingVelocity { -1 };
	int32 BI_PreviousMontagePosition { -1 };
	

#pragma endregion
	
	// Trajectory state functionality (input presence, acceleration synthesis for simulated proxies, etc.)
#pragma region Trajectory State
public:

	/// If true, this pawn is actively being provided with an input acceleration.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	bool IsInputPresent(bool bAllowGrace = false) const;

	/// If true, velocity differs enough from input acceleration that we're likely to pivot.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	bool DoInputAndVelocityDiffer() const { return FMath::Abs(InputVelocityOffset) > PivotPredictionAngleThreshold; }

	/// The angle, in degrees, that velocity differs from provided input (on the XY plane).
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	float InputVelocityOffsetAngle() const { return InputVelocityOffset; }

	/// The current effective acceleration we're moving at. This differs from input acceleration, as it's
	/// calculated from movement history.
	UFUNCTION(BlueprintPure, Category="Movement Trajectory")
	FVector GetCurrentEffectiveAcceleration() const { return CalculatedEffectiveAcceleration; }

private:

	void UpdateAllPredictions(float DeltaTime);

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

	/// Calls the starting prediction logic; the result will be cached in the TrajectoryIsStarting
	/// property.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	void UpdateStartPrediction(float DeltaTime);
	
	/// Check whether a stop is predicted, and store the prediction in OutStopPrediction. Only valid
	/// if UpdateStopPrediction has been called, or PrecalculateDistanceMatches is true.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	bool IsStopPredicted(FVector &OutStopPrediction) const;

	/// Check whether a pivot is predicted, and store the prediction in OutPivotPrediction. Only valid
	/// if UpdatePivotPrediction has been called, or PrecalculateDistanceMatches is true.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	bool IsPivotPredicted(FVector &OutPivotPrediction) const;

	/// Check whether we're starting to move or not.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	bool IsStarting() const { return bTrajectoryIsStarting; };
	
	/// If true, this component will pre-calculate stop and pivot predictions, so that they can be easily accessed
	/// in a thread-safe manner without needing to manually call the calculations each time.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory|Precalculations")
	bool bPrecalculateDistanceMatches { true };

	/// The angle of difference which we must exceed before a pivot can be predicted. Must be
	/// between 30 and 179. Recommended values are between 90 and 135 for normal use, though lower
	/// may be beneficial for motion matching.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory", meta=(UIMin="30", UIMax="179"))
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

	/// A position relative to our location where we predict a pivot. Only valid when UpdatePivotPrediction has
	/// been called, or PrecalculateDistanceMatches is true.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	bool bTrajectoryIsStarting { false };

	double LastStoppedTimestamp { 0 };
	bool bLastStoppedPivotCheck { false };
	FVector LastStartVelocityCheck { 0.f };
	
	/// Should we start with trajectory debug enabled? Only valid in editor.
	UPROPERTY(EditDefaultsOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	bool bDrawDebugPredictions { false };

	/// Should we draw *only* the trajectory, skipping pivot/stop markers? Only valid in editor.
	UPROPERTY(EditDefaultsOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	bool bDrawTrajectoryOnly { false };
	
	/// How long should our future predictions persist on screen for? -1 is 'one frame'. Only valid in editor.
	UPROPERTY(EditDefaultsOnly, Category="Movement Trajectory", meta=(AllowPrivateAccess=true))
	float DebugPredictionLifeTime { -1.f };
	
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
	FGMCE_MovementSampleCollection PredictMovementFuture(const FTransform& FromOrigin, const FRotator& ControllerRotation, const FQuat& MeshOffset, bool bIncludeHistory);

	/// Update the cached trajectory prediction. Called automatically if trajectory precalculation is enabled.
	UFUNCTION(BlueprintCallable, Category="Movement Trajectory")
	virtual void UpdateTrajectoryPrediction();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Movement Trajectory")
	FVector GetCurrentVelocityFromHistory();
	
	/// Should historical trajectory samples be taken?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory")
	bool bTrajectoryEnabled { true };

	/// Should the component automatically calculate a predicted trajectory and have it always available?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory|Precalculations")
	bool bPrecalculateFutureTrajectory { true };

	/// If true, trajectory will be calculated based on the character's skeletal mesh (if possible) rather than the
	/// root component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory")
    bool bTrajectoryUsesMesh { true };

	/// If true, trajectory will be calculated based on the controller rotation rather than the character rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory")
	bool bTrajectoryUsesControllerRotation { false };

	/// If true (and trajectory is using controller rotation), acceleration direction will stop rotating when it's
	/// a close match to the current controller rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory")
	bool bTrajectoryStopsAtControllerRotation { true };

	/// This value will be used to decay rotations in trajectory prediction. This does not
	/// apply to the acceleration rotation if you are using controller rotation and have
	/// trajectory stops at controller rotation set to true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Trajectory")
	float TrajectoryRotationDecay { 1.1f };
	
	/// The maximum size of the trajectory sample history.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	int32 MaxTrajectorySamples = { 200 };

	/// The maximum time-window of trajectory samples that should be kept.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	float TrajectoryHistorySeconds { 2.f };

	/// How long, in seconds, we should wait between samples. 0 will use a sane-but-frequent default.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	float TrajectoryHistoryPeriod { 0 };
	
	/// How many simulated samples should be generated for each second, when predicting trajectory?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	int32 TrajectorySimSampleRate = { 30 };

	/// How many seconds of prediction should we generate when predicting trajectory?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	float TrajectorySimSeconds = { 1.f };

	/// Whether predicted trajectory should adhere to the ground.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement Trajectory")
	bool bTrajectoryPredictCollisions { false };
	
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
	void GetCurrentAccelerationRotationVelocityFromHistory(FVector& OutAcceleration, FRotator& OutRotationVelocity, const EGMCE_TrajectoryRotationType& RotationType) const;

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

	UFUNCTION(BlueprintCallable, Category="Ragdoll")
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

	FVector LastRagdollBonePosition { 0.f };
	double LastRagdollTime { 0 };
	FVector RagdollInitialComponentOffset { 0.f };
	
private:

	void SetRagdollActive(bool bActive);

	bool IsRagdollBoneAuthority() const;

	int32 BI_ShouldReplicateRagdoll { -1 };
	int32 BI_RagdollBoneName { -1 };

	FVector CurrentRagdollGoal { 0.f };
	int32 BI_CurrentRagdollGoal { -1 };

	bool bRagdollStopped { false };
	int32 BI_RagdollStopped { -1 };
	
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

	void LeaveSolverMode();

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

	FGameplayTag PreviousSolverTag;

	UPROPERTY()
	TObjectPtr<UGMCE_BaseSolver> CurrentActiveSolver;

	UPROPERTY()
	UPrimitiveComponent* SolverAppliedBase { nullptr };
	
private:

	
	

#pragma endregion
	
};
