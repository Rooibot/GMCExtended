// Copyright 2024 Rooibot Games, LLC


#include "Solvers/GMCE_BaseSolver.h"

#include "GMCE_OrganicMovementCmp.h"

UGMCE_BaseSolver::UGMCE_BaseSolver()
{
}

void UGMCE_BaseSolver::SetupSolverInternal(UGMCE_OrganicMovementCmp* InMovementComponent)
{
	this->MovementComponent = InMovementComponent;
}

void UGMCE_BaseSolver::InitializeSolver()
{
	if (bUseBlueprintEvents) BlueprintInitializeSolver();
	NativeInitializeSolver();
}

bool UGMCE_BaseSolver::RunSolver(FSolverState& State, float DeltaTime)
{
	if (!MovementComponent) return false;
	bIsDebugActive = MovementComponent->ShouldDebugSolver(SolverMovementModeTag);

	if (bUseBlueprintEvents)
	{
		bool bResult = false;
		BlueprintRunSolver(State, DeltaTime, bResult);
		if (bResult) { return true; }
	}

	return NativeRunSolver(State, DeltaTime);	
}

FGameplayTag UGMCE_BaseSolver::GetPreferredSolverTag()
{
	FGameplayTag Result = FGameplayTag::EmptyTag;

	if (bUseBlueprintEvents)
	{
		BlueprintGetPreferredSolverTag(Result);
		if (Result.IsValid()) { return Result; }
	}

	return NativeGetPreferredSolverTag();	
}

UPrimitiveComponent* UGMCE_BaseSolver::GetSolverBase(FSolverState& State)
{
	UPrimitiveComponent* Result = nullptr;

	if (bUseBlueprintEvents)
	{
		BlueprintGetSolverBase(State, Result);		
	}

	if (Result == nullptr)
	{
		Result = NativeGetSolverBase(State);
	}

	return Result;	
}

void UGMCE_BaseSolver::PreProcessInput(FSolverState& State)
{
	if (bUseBlueprintEvents)
	{
		BlueprintPreProcessInput(State);
	}
	NativePreProcessInput(State);		
}

bool UGMCE_BaseSolver::PerformMovement(FSolverState& State, float DeltaTime)
{
	if (bUseBlueprintEvents)
	{
		bool bResult = false;
		BlueprintPerformMovement(State, DeltaTime, bResult);
		if (bResult) { return true; }
	}
	
	return NativePerformMovement(State, DeltaTime);	
}

void UGMCE_BaseSolver::DrawDebugConnector(const FVector& StartPoint, const FVector& EndPoint, const FColor& Color,
	float SphereRadius, float LineThickness) const
{
#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	if (!bIsDebugActive) return;

	const UWorld* World = MovementComponent->GetWorld();
	const FVector LINE_OFFSET = FVector::UpVector;
	DrawDebugSphere(World, StartPoint, SphereRadius, 18, Color, false, -1, 0, LineThickness);
	DrawDebugSphere(World, EndPoint, SphereRadius, 18, Color, false, -1, 0, LineThickness);
	DrawDebugLine(World, StartPoint + LINE_OFFSET, EndPoint + LINE_OFFSET, Color, false, -1, 0, LineThickness);
#endif	
}

void UGMCE_BaseSolver::DrawDebugPointNormal(const FVector& Point, const FVector& Normal, const FColor& Color,
	float SphereRadius, float LineThickness) const
{
#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	if (!bIsDebugActive) return;

	const UWorld* World = MovementComponent->GetWorld();
	DrawDebugSphere(World, Point, SphereRadius, 18, Color, false, -1, 0, LineThickness);
	DrawDebugLine(World, Point, Point + (Normal * SphereRadius * 2.f), Color, false, -1, 0, LineThickness);
#endif	
}

void UGMCE_BaseSolver::DrawDebugPointAngle(const FVector& Point, const FVector& Direction1, const FVector& Direction2,
	const FColor& Color, float SphereRadius, float LineThickness) const
{
#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	if (!bIsDebugActive) return;

	const FVector AnglePoint1 = Point + (Direction1.GetSafeNormal() * SphereRadius * 2.f);
	const FVector AnglePoint2 = Point + (Direction2.GetSafeNormal() * SphereRadius * 2.f);
	
	const UWorld* World = MovementComponent->GetWorld();
	DrawDebugSphere(World, Point, SphereRadius, 18, Color, false, -1, 0, LineThickness);
	DrawDebugLine(World, Point, AnglePoint1, Color, false, -1, 0, LineThickness);
	DrawDebugLine(World, Point, AnglePoint2, Color, false, -1, 0, LineThickness);
#endif	
}

void UGMCE_BaseSolver::NativeInitializeSolver()
{
}

bool UGMCE_BaseSolver::NativeRunSolver(FSolverState& State, float DeltaTime)
{
	return false;
}

FGameplayTag UGMCE_BaseSolver::NativeGetPreferredSolverTag()
{
	return SolverMovementModeTag;
}

UPrimitiveComponent* UGMCE_BaseSolver::NativeGetSolverBase(FSolverState& State)
{
	return nullptr;
}

void UGMCE_BaseSolver::NativePreProcessInput(FSolverState& State)
{
}

bool UGMCE_BaseSolver::NativePerformMovement(FSolverState& State, float DeltaTime)
{
	return false;
}

