// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GMCOrganicMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GMCE_BaseSolver.generated.h"

class UGMCE_OrganicMovementCmp;

DECLARE_LOG_CATEGORY_EXTERN(LogGMCExtendedSolver, Log, All)

UENUM(BlueprintType)
enum class EGMCExtendedLogType : uint8
{
	DebugLog UMETA(DisplayName = "Debug Only Log"),
	LogVeryVerbose UMETA(DisplayName = "Very Verbose Log"),
	LogVerbose UMETA(DisplayName = "Verbose Log"),
	LogWarning UMETA(DisplayName = "Warning Log"),
	LogError UMETA(DisplayName = "Error Log")
};

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FSolverState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	bool bIsPrediction { false };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	EGMC_MovementMode MovementMode { EGMC_MovementMode::Grounded };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FGameplayTag MovementTag { FGameplayTag::EmptyTag };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector Location { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FRotator Rotation { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector LinearVelocity { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector RawInput { 0.f };

	// -- Values below here can be changed by a solver and will be acted on.
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solver State")
	FVector ProcessedInput { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solver State")
	FGameplayTagContainer AvailableSolvers { };
	
};


/**
 * 
 */
UCLASS(Blueprintable, DisplayName="GMCExtended Movement Solver")
class GMCEXTENDED_API UGMCE_BaseSolver : public UObject
{
	GENERATED_BODY()

public:
	UGMCE_BaseSolver();

	void SetupSolverInternal(UGMCE_OrganicMovementCmp* MovementComponent);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GMC Extended|Solvers")
	FGameplayTag GetTag() const { return SolverMovementModeTag; }

	/**
	 * Initializes this solver.
	 * Called when the movement component is ready, to allow the solver to set up any initial state it might need.
	 * Calls BlueprintInitializeSolver and NativeInitializeSolver under the hood.
	 */
	void InitializeSolver();

	/**
	 * Informs the solver that a given movement tag has become active. This will be called when the active
	 * movement tag changes, even if the solver handles both tags.
	 * @param ActiveMovementTag The tag which has become active.
	 */
	void ActivateSolver(const FGameplayTag& ActiveMovementTag);
	
	/**
	 * Runs the solver to determine if it has any viable movement options.
	 * Given the current Parcore state, determine if this solver has any viable moves available to it. Any
	 * relevant information should be cached in this event for use in actual solver movement logic. This is also
	 * a viable place to handle any debug visualizations. Under the hood, this calls BlueprintRunSolver, and if
	 * that returns false, it will call NativeRunSolver.
	 * @param State The current Parcore state.
	 * @param DeltaTime The time since RunSolver was last called.
	 * @return true if this solver has a movement solution, false otherwise.
	 */
	bool RunSolver(FSolverState& State, float DeltaTime);

	/**
	 * Obtains a preferred solver tag for this solver, as of the last time RunSolver was run. This can be used so
	 * that a solver such as the Climbing Solver can return Solver.Climbing.FreeClimb or Solver.Climbing.Mantle
	 * instead of just the baseline Solver.Climbing tag. Behind the scenes, calls BlueprintGetPreferredSolverTag,
	 * and if that doesn't return a valid tag, will call NativeGetPreferredSolverTag.
	 * @return A preferred tag to be used as the 'current active solver' value.
	 */
	FGameplayTag GetPreferredSolverTag();

	/**
	 * Gives the active solver the option to set or override the component on which an actor's movement should
	 * be based.
	 * @return A component, or nullptr.
	 */
	UPrimitiveComponent* GetSolverBase(FSolverState& State);
	
	/**
	 * If a solver is controlling the active movement, this will be called to allow it to pre-process any input
	 * before the main movement loop. Under the hood, this will call BlueprintPreProcessInput followed by
	 * NativePreProcessInput.
	 * @param State The current Parcore state.
	 */
	void PreProcessInput(FSolverState& State);

	/**
	 * If a solver is controlling the active movement, this will be called during the main movement loop to allow it
	 * to actually change the character state. Under the hood, this calls BlueprintPerformMovement, and if it
	 * returns false, will call NativePerformMovement.
	 * @param State The current Parcore state.
	 * @param DeltaTime Time elapsed since the last movement action.
	 * @return true if the solver wants to retain control of movement, false if it should surrender it after this cycle.
	 */
	bool PerformMovement(FSolverState& State, float DeltaTime);

	void MovementUpdate(FSolverState& State, float DeltaTime);

	void MovementUpdateSimulated(FSolverState& State, float DeltaTime);

	void PostMovementUpdate(FSolverState& State, float DeltaTime);
	
	void PostMovementUpdateSimulated(FSolverState& State, float DeltaTime);

	void PredictionTick(FSolverState& State, float DeltaTime);

	void SimulationTick(FSolverState& State, float DeltaTime);

	void AncillaryTick(FSolverState& State, float DeltaTime);

	void DeactivateSolver();

protected:

	// Visualization helpers.

	/// Draws a debug connector between two points. Will only do anything if in a debug build and with this solver
	/// set to debug.
	UFUNCTION(BlueprintCallable, DisplayName="Draw Debug Connector", Category="GMC Extended|Debug")
	void DrawDebugConnector_BP(const FVector StartPoint, const FVector EndPoint, const FLinearColor Color, float SphereRadius, float LineThickness);
	
	void DrawDebugConnector(const FVector& StartPoint, const FVector& EndPoint, const FColor& Color, float SphereRadius, float LineThickness) const;

	
	/// Draws a sphere with a vector indicating a normal. Will only do anything if in a debug build and with this solver
	/// set to debug.
	UFUNCTION(BlueprintCallable, DisplayName="Draw Debug Point with Normal", Category="GMC Extended|Debug")
	void DrawDebugPointNormal_BP(const FVector Point, const FVector Normal, const FLinearColor Color, float SphereRadius, float LineThickness);
	
	void DrawDebugPointNormal(const FVector& Point, const FVector& Normal, const FColor& Color, float SphereRadius, float LineThickness) const;

	/// Draws a sphere with two vectors indicating angles. Will only do anything if in a debug build and with this solver
	/// set to debug.
	UFUNCTION(BlueprintCallable, DisplayName="Draw Debug Point and Angles", Category="GMC Extended|Debug")
	void DrawDebugPointAngle_BP(const FVector Point, const FVector Direction1, const FVector Direction2, const FLinearColor Color, float SphereRadius, float LineThickness);
	
	void DrawDebugPointAngle(const FVector& Point, const FVector& Direction1, const FVector& Direction2, const FColor& Color, float SphereRadius, float LineThickness) const;

	/// Draws a sphere. Will only do anything if in a debug build and with this solver set to debug.
	UFUNCTION(BlueprintCallable, DisplayName="Draw Debug Sphere", Category="GMC Extended|Debug")
	void DrawDebugSphere_BP(const FVector Origin, float SphereRadius, int Segments, const FLinearColor Color, float LineThickness);

	/// Draws a line. Will only do anything if in a debug build and with this solver set to debug.
	UFUNCTION(BlueprintCallable, DisplayName="Draw Debug Line", Category="GMC Extended|Debug")
	void DrawDebugLine_BP(const FVector Start, const FVector End, const FLinearColor Color, float LineThickness);

	/// Output text to the Unreal log (and optionally the screen).
	UFUNCTION(BlueprintCallable, DisplayName="Print String to Solver Log", Category="GMC Extended|Debug")
	void SolverLogString(const FString Message, EGMCExtendedLogType SolverLogType, bool bShowOnScreen);

	/// Output text to the Unreal log (and optionally the screen).
	UFUNCTION(BlueprintCallable, DisplayName="Print Text to Solver Log", Category="GMC Extended|Debug")
	void SolverLogText(const FText Message, EGMCExtendedLogType SolverLogType, bool bShowOnScreen);
	

	UFUNCTION(BlueprintCallable, DisplayName="Swap Server State (if Necessary)", Category="GMC Extended")
	void ServerSwapStateIfNeeded();
	
	// ------ NATIVE
#pragma region Native
public:
	/**
	 * Native implementation of InitializeSolver.
	 * Called when the movement component is ready, to allow the solver to setup any initial state it might need.
	 */
	virtual void NativeInitializeSolver();

	/**
	 * Native implementation of ActivateSolver.
	 * @param ActiveMovementTag The tag which has become active.
	 */
	virtual void NativeActivateSolver(const FGameplayTag& ActiveMovementTag);
	
	/**
	 * Native implementation of RunSolver.
	 * @param State The current Parcore movement state.
	 * @param DeltaTime The time since RunSolver was last called.
	 * @return true if this solver would like to handle movement, otherwise false.
	 */
	virtual bool NativeRunSolver(FSolverState& State, float DeltaTime);

	virtual FGameplayTag NativeGetPreferredSolverTag();

	virtual UPrimitiveComponent* NativeGetSolverBase(FSolverState& State);
	
	/**
	 * Native implementation of PreProcessInput.
	 * @param State The current Parcore state.
	 */
	virtual void NativePreProcessInput(FSolverState& State);

	/**
	 * Native implementation of PerformMovement.
	 * @param State The current Parcore state.
	 * @param DeltaTime The time elapsed since the last movement action.
	 * @return true if the solver wishes to retain control of movement, otherwise false to surrender it.
	 */
	virtual bool NativePerformMovement(FSolverState& State, float DeltaTime);

	virtual void NativeMovementUpdate(FSolverState& State, float DeltaTime);

	virtual void NativeMovementUpdateSimulated(FSolverState& State, float DeltaTime);

	virtual void NativePostMovementUpdate(FSolverState& State, float DeltaTime);
	
	virtual void NativePostMovementUpdateSimulated(FSolverState& State, float DeltaTime);

	virtual void NativeSimulationTick(FSolverState& State, float DeltaTime);

	virtual void NativePredictionTick(FSolverState& State, float DeltaTime);

	virtual void NativeAncillaryTick(FSolverState& State, float DeltaTime);

	virtual void NativeDeactivateSolver();
	
#pragma endregion
	
	// ------ BLUEPRINT
#pragma region Blueprint
public:
	/**
	 * Blueprint implementation of solver initialization.
	 * Called by GMCEx when the movement component is ready, to allow a solver to setup any initial state it might
	 * need.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Initialize Solver", Category="GMC Extended|Solvers")
	void BlueprintInitializeSolver();

	/**
	 * Blueprint implementation of solver initialization.
	 * Called by GMCEx when the movement component is ready, to allow a solver to setup any initial state it might
	 * need.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Activate Solver", Category="GMC Extended|Solvers")
	void BlueprintActivateSolver(FGameplayTag GameplayTag);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Get Preferred Solver Tag", Category="GMC Extended|Solvers")
	void BlueprintGetPreferredSolverTag(UPARAM(DisplayName="Preferred Tag") FGameplayTag& OutTag);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Get Actor Base", Category="GMC Extended|Solvers")
	void BlueprintGetSolverBase(UPARAM(ref) FSolverState& State, UPARAM(DisplayName="New Actor Base") UPrimitiveComponent*& Component);
	
	/**
	 * Blueprint implementation of solver validity check.
	 * @param State Current Parcore state
	 * @param OutResult true if the solver can take control of movement, false otherwise.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Run Solver", Category="GMC Extended|Solvers")
	void BlueprintRunSolver(UPARAM(ref) FSolverState& State, float DeltaTime, UPARAM(DisplayName="Offer control") bool& OutResult);

	/**
	 * Blueprint implementation of PreProcessInput.
	 * @param State Current Parcore state
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Pre-Process Input", Category="GMC Extended|Solvers")
	void BlueprintPreProcessInput(UPARAM(ref) FSolverState& State);

	/**
	 * Blueprint implementation of main movement handler, PerformMovement.
	 * @param State The current Parcore state
	 * @param DeltaTime The time since the last movement action.
	 * @param OutResult Set this to true to retain control, false otherwise.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Perform Movement", Category="GMC Extended|Solvers")
	void BlueprintPerformMovement(UPARAM(ref) FSolverState& State, float DeltaTime, UPARAM(DisplayName="Retain control") bool& OutResult);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Movement Update", Category="GMC Extended|Solvers")
	void BlueprintMovementUpdate(UPARAM(ref) FSolverState& State, float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Simulated Movement Update", Category="GMC Extended|Solvers")
	void BlueprintMovementUpdateSimulated(UPARAM(ref) FSolverState& State, float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Post Movement Update", Category="GMC Extended|Solvers")
	void BlueprintPostMovementUpdate(UPARAM(ref) FSolverState& State, float DeltaTime);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Post Simulated Movement Update", Category="GMC Extended|Solvers")
	void BlueprintPostMovementUpdateSimulated(UPARAM(ref) FSolverState& State, float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Simulation Tick", Category="GMC Extended|Solvers")
	void BlueprintSimulationTick(UPARAM(ref) FSolverState& State, float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Prediction Tick", Category="GMC Extended|Solvers")
	void BlueprintPredictionTick(UPARAM(ref) FSolverState& State, float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Ancillary Tick", Category="GMC Extended|Solvers")
	void BlueprintAncillaryTick(UPARAM(ref) FSolverState& State, float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Deactivate Solver", Category="GMC Extended|Solvers")
	void BlueprintDeactivateSolver();
	
	UGMCE_OrganicMovementCmp* GetMovementComponent() const { return MovementComponent; }

#pragma endregion 

	// ------ Trace Wrappers
#pragma region Traces

public:
	/**
	 * Does a collision trace along the given line and returns the first blocking hit encountered.
	 * This trace finds the objects that RESPONDS to the given TraceChannel
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param TraceChannel	
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHit		Properties of the trace hit.
	 * @return				True if there was a hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName="Line Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="raycast"))
	bool LineTraceSingle(const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	
	/**
	 * Does a collision trace along the given line and returns all hits encountered up to and including the first blocking hit.
	 * This trace finds the objects that RESPOND to the given TraceChannel
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param TraceChannel	The channel to trace
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHit		Properties of the trace hit.
	 * @return				True if there was a blocking hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Multi Line Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="raycast"))
	bool LineTraceMulti(const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	
	/**
	 * Sweeps a sphere along the given line and returns the first blocking hit encountered.
	 * This trace finds the objects that RESPONDS to the given TraceChannel
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the sphere to sweep
	 * @param TraceChannel	
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHit		Properties of the trace hit.
	 * @return				True if there was a hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Sphere Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool SphereTraceSingle(const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Sweeps a sphere along the given line and returns all hits encountered up to and including the first blocking hit.
	 * This trace finds the objects that RESPOND to the given TraceChannel
	 * 
	 * @param WorldContext	World context
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the sphere to sweep
	 * @param TraceChannel	
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	 * @return				True if there was a blocking hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Multi Sphere Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool SphereTraceMulti(const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Sweeps a box along the given line and returns the first blocking hit encountered.
	* This trace finds the objects that RESPONDS to the given TraceChannel
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param HalfSize	    Distance from the center of box along each axis
	* @param Orientation	Orientation of the box
	* @param TraceChannel
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit			Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Box Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool BoxTraceSingle(const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Sweeps a box along the given line and returns all hits encountered.
	* This trace finds the objects that RESPONDS to the given TraceChannel
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param HalfSize	    Distance from the center of box along each axis
	* @param Orientation	Orientation of the box
	* @param TraceChannel
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHits		A list of hits, sorted along the trace from start to finish. The blocking hit will be the last hit, if there was one.
	* @return				True if there was a blocking hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Box Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool BoxTraceMulti(const FVector Start, const FVector End, FVector HalfSize, const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Sweeps a capsule along the given line and returns the first blocking hit encountered.
	 * This trace finds the objects that RESPOND to the given TraceChannel
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the capsule to sweep
	 * @param HalfHeight	Distance from center of capsule to tip of hemisphere endcap.
	 * @param TraceChannel	
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHit		Properties of the trace hit.
	 * @return				True if there was a hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Capsule Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool CapsuleTraceSingle(const FVector Start, const FVector End, float Radius, float HalfHeight, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);


	/**
	 * Sweeps a capsule along the given line and returns all hits encountered up to and including the first blocking hit.
	 * This trace finds the objects that RESPOND to the given TraceChannel
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the capsule to sweep
	 * @param HalfHeight	Distance from center of capsule to tip of hemisphere endcap.
	 * @param TraceChannel	
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	 * @return				True if there was a blocking hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Multi Capsule Trace By Channel", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool CapsuleTraceMulti(const FVector Start, const FVector End, float Radius, float HalfHeight, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	
	/**
	 * Sweeps a sphere along the given line and returns the first hit encountered.
	 * This only finds objects that are of a type specified by ObjectTypes.
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the sphere to sweep
	 * @param ObjectTypes	Array of Object Types to trace 
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHit		Properties of the trace hit.
	 * @return				True if there was a hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Sphere Trace For Objects", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool SphereTraceSingleForObjects(const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Sweeps a sphere along the given line and returns all hits encountered.
	 * This only finds objects that are of a type specified by ObjectTypes.
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the sphere to sweep
	 * @param ObjectTypes	Array of Object Types to trace 
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	 * @return				True if there was a hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Multi Sphere Trace For Objects", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool SphereTraceMultiForObjects(const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	

	/**
	* Sweeps a box along the given line and returns the first hit encountered.
	* This only finds objects that are of a type specified by ObjectTypes.
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Orientation	
	* @param HalfSize		Radius of the sphere to sweep
	* @param ObjectTypes	Array of Object Types to trace
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit			Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Box Trace For Objects", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool BoxTraceSingleForObjects(const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);


	/**
	* Sweeps a box along the given line and returns all hits encountered.
	* This only finds objects that are of a type specified by ObjectTypes.
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Orientation
	* @param HalfSize		Radius of the sphere to sweep
	* @param ObjectTypes	Array of Object Types to trace
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Box Trace For Objects", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool BoxTraceMultiForObjects(const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Sweeps a capsule along the given line and returns the first hit encountered.
	 * This only finds objects that are of a type specified by ObjectTypes.
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the capsule to sweep
	 * @param HalfHeight	Distance from center of capsule to tip of hemisphere endcap.
	 * @param ObjectTypes	Array of Object Types to trace 
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHit		Properties of the trace hit.
	 * @return				True if there was a hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Capsule Trace For Objects", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool CapsuleTraceSingleForObjects(const FVector Start, const FVector End, float Radius, float HalfHeight, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Sweeps a capsule along the given line and returns all hits encountered.
	 * This only finds objects that are of a type specified by ObjectTypes.
	 * 
	 * @param Start			Start of line segment.
	 * @param End			End of line segment.
	 * @param Radius		Radius of the capsule to sweep
	 * @param HalfHeight	Distance from center of capsule to tip of hemisphere endcap.
	 * @param ObjectTypes	Array of Object Types to trace 
	 * @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	 * @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	 * @return				True if there was a hit, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Collision", meta=(bIgnoreSelf="true", AutoCreateRefTerm="ActorsToIgnore", DisplayName = "Multi Capsule Trace For Objects", AdvancedDisplay="TraceColor,TraceHitColor,DrawTime", Keywords="sweep"))
	bool CapsuleTraceMultiForObjects(const FVector Start, const FVector End, float Radius, float HalfHeight, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	// BY PROFILE

	/**
	* Trace a ray against the world using a specific profile and return the first blocking hit
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit			Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Line Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	bool LineTraceSingleByProfile(const FVector Start, const FVector End, UPARAM(Meta=(GetOptions="Engine.KismetSystemLibrary.GetCollisionProfileNames")) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	*  Trace a ray against the world using a specific profile and return overlapping hits and then first blocking hit
	*  Results are sorted, so a blocking hit (if found) will be the last element of the array
	*  Only the single closest blocking result will be generated, no tests will be done after that
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit		Properties of the trace hit.
	* @return				True if there was a blocking hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Line Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	bool LineTraceMultiByProfile(const FVector Start, const FVector End, UPARAM(Meta=(GetOptions=GetCollisionProfileNames)) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	*  Sweep a sphere against the world and return the first blocking hit using a specific profile
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius			Radius of the sphere to sweep
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit			Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Sphere Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	bool SphereTraceSingleByProfile(const FVector Start, const FVector End, float Radius, UPARAM(Meta=(GetOptions=GetCollisionProfileNames)) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	*  Sweep a sphere against the world and return all initial overlaps using a specific profile, then overlapping hits and then first blocking hit
	*  Results are sorted, so a blocking hit (if found) will be the last element of the array
	*  Only the single closest blocking result will be generated, no tests will be done after that
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius		Radius of the sphere to sweep
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	* @return				True if there was a blocking hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Sphere Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	bool SphereTraceMultiByProfile(const FVector Start, const FVector End, float Radius, UPARAM(Meta=(GetOptions=GetCollisionProfileNames)) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	*  Sweep a box against the world and return the first blocking hit using a specific profile
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param HalfSize	    Distance from the center of box along each axis
	* @param Orientation	Orientation of the box
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit			Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Box Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	bool BoxTraceSingleByProfile(const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, UPARAM(Meta=(GetOptions=GetCollisionProfileNames)) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	*  Sweep a box against the world and return all initial overlaps using a specific profile, then overlapping hits and then first blocking hit
	*  Results are sorted, so a blocking hit (if found) will be the last element of the array
	*  Only the single closest blocking result will be generated, no tests will be done after that
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param HalfSize	    Distance from the center of box along each axis
	* @param Orientation	Orientation of the box
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHits		A list of hits, sorted along the trace from start to finish. The blocking hit will be the last hit, if there was one.
	* @return				True if there was a blocking hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Box Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	bool BoxTraceMultiByProfile(const FVector Start, const FVector End, FVector HalfSize, const FRotator Orientation, UPARAM(Meta=(GetOptions=GetCollisionProfileNames)) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);


	/**
	*  Sweep a capsule against the world and return the first blocking hit using a specific profile
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius			Radius of the capsule to sweep
	* @param HalfHeight		Distance from center of capsule to tip of hemisphere endcap.
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit			Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Capsule Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	bool CapsuleTraceSingleByProfile(const FVector Start, const FVector End, float Radius, float HalfHeight, UPARAM(Meta=(GetOptions=GetCollisionProfileNames)) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	*  Sweep a capsule against the world and return all initial overlaps using a specific profile, then overlapping hits and then first blocking hit
	*  Results are sorted, so a blocking hit (if found) will be the last element of the array
	*  Only the single closest blocking result will be generated, no tests will be done after that
	*
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius			Radius of the capsule to sweep
	* @param HalfHeight		Distance from center of capsule to tip of hemisphere endcap.
	* @param ProfileName	The 'profile' used to determine which components to hit
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	* @return				True if there was a blocking hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Capsule Trace By Profile", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	bool CapsuleTraceMultiByProfile(const FVector Start, const FVector End, float Radius, float HalfHeight, UPARAM(Meta=(GetOptions=GetCollisionProfileNames)) FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	
#pragma endregion

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Solver");
	bool bUseBlueprintEvents { true };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Solver")
	bool bIsDebugActive { false };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Solver")
	TObjectPtr<AGMC_Pawn> Owner;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Solver")
	TObjectPtr<UGMCE_OrganicMovementCmp> MovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, DisplayName="Movement Tag", Category="Solver");
	FGameplayTag SolverMovementModeTag;

};


