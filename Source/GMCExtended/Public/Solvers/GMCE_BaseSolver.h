// Copyright 2024 Rooibot Games, LLC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GMCOrganicMovementComponent.h"
#include "GMCE_BaseSolver.generated.h"

class UGMCE_OrganicMovementCmp;

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FSolverState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsPrediction { false };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EGMC_MovementMode MovementMode { EGMC_MovementMode::Grounded };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag MovementTag { FGameplayTag::EmptyTag };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Location { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator Rotation { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LinearVelocity { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector RawInput { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector ProcessedInput { 0.f };

	// -- Values below here can be changed by a solver and will be acted on.
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer AvailableSolvers { };
	
};


/**
 * 
 */
UCLASS()
class GMCEXTENDED_API UGMCE_BaseSolver : public UObject
{
	GENERATED_BODY()

public:
	UGMCE_BaseSolver();

	void SetupSolverInternal(UGMCE_OrganicMovementCmp* MovementComponent);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
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
	UFUNCTION(BlueprintCallable)
	void DrawDebugConnector(const FVector& StartPoint, const FVector& EndPoint, const FColor& Color, float SphereRadius, float LineThickness) const;

	UFUNCTION(BlueprintCallable)
	void DrawDebugPointNormal(const FVector& Point, const FVector& Normal, const FColor& Color, float SphereRadius, float LineThickness) const;

	UFUNCTION(BlueprintCallable)
	void DrawDebugPointAngle(const FVector& Point, const FVector& Direction1, const FVector& Direction2, const FColor& Color, float SphereRadius, float LineThickness) const;
	
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
	 * Called by Parcore when the movement component is ready, to allow a solver to setup any initial state it might
	 * need.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Initialize Solver")
	void BlueprintInitializeSolver();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Get Preferred Solver Tag")
	void BlueprintGetPreferredSolverTag(UPARAM(DisplayName="Preferred Tag") FGameplayTag& OutTag);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Get Actor Base")
	void BlueprintGetSolverBase(UPARAM(ref) FSolverState& State, UPARAM(DisplayName="New Actor Base") UPrimitiveComponent*& Component);
	
	/**
	 * Blueprint implementation of solver validity check.
	 * @param State Current Parcore state
	 * @param OutResult true if the solver can take control of movement, false otherwise.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Run Solver")
	void BlueprintRunSolver(UPARAM(ref) FSolverState& State, float DeltaTime, UPARAM(DisplayName="Offer control") bool& OutResult);

	/**
	 * Blueprint implementation of PreProcessInput.
	 * @param State Current Parcore state
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Pre-Process Input")
	void BlueprintPreProcessInput(UPARAM(ref) FSolverState& State);

	/**
	 * Blueprint implementation of main movement handler, PerformMovement.
	 * @param State The current Parcore state
	 * @param DeltaTime The time since the last movement action.
	 * @param OutResult Set this to true to retain control, false otherwise.
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Perform Movement")
	void BlueprintPerformMovement(UPARAM(ref) FSolverState& State, float DeltaTime, UPARAM(DisplayName="Retain control") bool& OutResult);
	
	UGMCE_OrganicMovementCmp* GetMovementComponent() const { return MovementComponent; }

#pragma endregion 
	
protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly);
	bool bUseBlueprintEvents { true };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	bool bIsDebugActive { false };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TObjectPtr<UGMCE_OrganicMovementCmp> MovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, DisplayName="Movement Tag");
	FGameplayTag SolverMovementModeTag;
	
};
