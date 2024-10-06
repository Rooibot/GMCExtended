// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GMCOrganicMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GMCE_BaseSolver.generated.h"

class UGMCE_OrganicMovementCmp;
class UGMCE_SolverMontageWrapper;	
	
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector ProcessedInput { 0.f };

	// -- Values below here can be changed by a solver and will be acted on.
	
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
	
	// ------ NATIVE
#pragma region Native
public:
	/**
	 * Native implementation of InitializeSolver.
	 * Called when the movement component is ready, to allow the solver to setup any initial state it might need.
	 */
	virtual void NativeInitializeSolver();

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

	// ------ Montage Helpers
#pragma region Montage

public:
	UFUNCTION(BlueprintCallable, Category = "GMC Extended|Solver Utilities")
	int PlayMontageBlocking(USkeletalMeshComponent* SkeletalMeshComponent, UAnimMontage* Montage, float StartPosition, float PlayRate);

	UFUNCTION(BlueprintNativeEvent)
	void OnMontageStart(int MontageHandle, bool bIsCosmeticOnly);

	UFUNCTION(BlueprintNativeEvent)
	void OnMontageBlendInDone(int MontageHandle, bool bIsCosmeticOnly);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnMontageBlendOutStarted(int MontageHandle, bool bIsCosmeticOnly);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnMontageComplete(int MontageHandle, bool bIsCosmeticOnly);

	UFUNCTION(BlueprintNativeEvent)
	void OnMontageInterrupted(int MontageHandle, bool bIsCosmeticOnly);

	void RemoveMontageWrapper(UGMCE_SolverMontageWrapper* Wrapper);
	
private:

	TArray<UGMCE_SolverMontageWrapper*> MontageWrappers;
	
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

UCLASS()
class GMCEXTENDED_API UGMCE_SolverMontageWrapper : public UObject
{
	GENERATED_BODY()

public:

	static UGMCE_SolverMontageWrapper* PlayMontageBlocking(UGMCE_BaseSolver* Solver, USkeletalMeshComponent* SkeletalMeshComponent, UAnimMontage* Montage, float StartPosition, float PlayRate)
	{
		static int Counter = 0;

		if (!IsValid(Solver)) return nullptr;
		if (!IsValid(SkeletalMeshComponent)) return nullptr;
		if (!IsValid(Montage)) return nullptr;

		UGMCE_SolverMontageWrapper* NewWrapper = NewObject<UGMCE_SolverMontageWrapper>(Solver);
		NewWrapper->Setup(Solver, Counter++);
		if (NewWrapper->PlayMontage(SkeletalMeshComponent, Montage, StartPosition, PlayRate)) return NewWrapper;

		return nullptr;
	}
	
	void Setup(UGMCE_BaseSolver* NewSolver, int NewHandle)
	{
		Solver = NewSolver;
		WrapperHandle = NewHandle;
		Delegate_OnMontageStart.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageStart);
		Delegate_OnMontageBlendInComplete.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageBlendInComplete);
		Delegate_OnMontageBlendOutBegin.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageBlendOutBegin);
		Delegate_OnMontageComplete.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageComplete);

		Delegate_OnMontageStarted_Cosmetic.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageStarted_Cosmetic);
		Delegate_OnMontageBlendedInEnded_Cosmetic.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageBlendInComplete_Cosmetic);
		Delegate_OnMontageBlendingOutStarted_Cosmetic.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageBlendOutBegin_Cosmetic);
		Delegate_OnMontageEnded_Cosmetic.BindUObject(this, &UGMCE_SolverMontageWrapper::OnMontageComplete_Cosmetic);
	}

	bool PlayMontage(USkeletalMeshComponent* SkeletalMeshComponent, UAnimMontage* Montage, float StartPosition, float PlayRate);
	
	UPROPERTY()
	int WrapperHandle;

protected:

	UPROPERTY()
	UGMCE_BaseSolver* Solver;
	
	bool bWasInterruptedBeforeComplete;
	
	FGMC_OnMontageStart						Delegate_OnMontageStart;
	FGMC_OnMontageBlendInComplete			Delegate_OnMontageBlendInComplete;
	FGMC_OnMontageBlendOutBegin				Delegate_OnMontageBlendOutBegin;
	FGMC_OnMontageComplete					Delegate_OnMontageComplete;

	FOnMontageStarted						Delegate_OnMontageStarted_Cosmetic;
	FOnMontageBlendedInEnded				Delegate_OnMontageBlendedInEnded_Cosmetic;
	FOnMontageBlendingOutStarted			Delegate_OnMontageBlendingOutStarted_Cosmetic;
	FOnMontageEnded							Delegate_OnMontageEnded_Cosmetic;
	
	UFUNCTION()
	void OnMontageStart() const
	{
		if (IsValid(Solver)) Solver->OnMontageStart(WrapperHandle, false);
	}

	UFUNCTION()
	void OnMontageBlendInComplete() const
	{
		if (IsValid(Solver)) Solver->OnMontageBlendInDone(WrapperHandle, false);
	}

	UFUNCTION()
	void OnMontageBlendOutBegin() const
	{
		if (IsValid(Solver)) Solver->OnMontageBlendOutStarted(WrapperHandle, false);
	}

	UFUNCTION()
	void OnMontageComplete()
	{
		if (!IsValid(Solver)) return;
		
		Solver->OnMontageComplete(WrapperHandle, false);
		Solver->RemoveMontageWrapper(this);		
	}

	UFUNCTION()
	void OnMontageStarted_Cosmetic(UAnimMontage* Montage)
	{
		if (IsValid(Solver)) Solver->OnMontageStart(WrapperHandle, true);
	}

	void OnMontageBlendInComplete_Cosmetic(UAnimMontage* Montage)
	{
		if (IsValid(Solver)) Solver->OnMontageBlendInDone(WrapperHandle, true);
	}
	
	UFUNCTION()
	void OnMontageBlendOutBegin_Cosmetic(UAnimMontage* Montage, bool bInterrupted)
	{
		if (!bInterrupted)
		{
			if (IsValid(Solver)) Solver->OnMontageBlendOutStarted(WrapperHandle, true);
		}
		else if (!bWasInterruptedBeforeComplete)
		{
			bWasInterruptedBeforeComplete = true;
			if (IsValid(Solver)) Solver->OnMontageInterrupted(WrapperHandle, true);
		}
	}

	UFUNCTION()
	void OnMontageComplete_Cosmetic(UAnimMontage* Montage, bool bInterrupted)
	{
		if (!bInterrupted)
		{
			if (IsValid(Solver)) Solver->OnMontageComplete(WrapperHandle, true);
		}
		else if (!bWasInterruptedBeforeComplete)
		{
			bWasInterruptedBeforeComplete = true;
			if (IsValid(Solver)) Solver->OnMontageInterrupted(WrapperHandle, true);
		}
	}
	
};