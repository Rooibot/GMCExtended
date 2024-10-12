﻿// Copyright 2024 Rooibot Games, LLC


#include "Solvers/GMCE_BaseSolver.h"
#include "GMCE_OrganicMovementCmp.h"

DEFINE_LOG_CATEGORY(LogGMCExtendedSolver)

UGMCE_BaseSolver::UGMCE_BaseSolver()
{
}

void UGMCE_BaseSolver::SetupSolverInternal(UGMCE_OrganicMovementCmp* InMovementComponent)
{
	this->MovementComponent = InMovementComponent;
	this->Owner = InMovementComponent->GetGMCPawnOwner();

	Delegate_OnMontageStart.BindUObject(this, &UGMCE_BaseSolver::OnMontageStart_Networked_Internal);
	Delegate_OnMontageBlendInComplete.BindUObject(this, &UGMCE_BaseSolver::OnMontageBlendInDone_Networked_Internal);
	Delegate_OnMontageBlendOutBegin.BindUObject(this, &UGMCE_BaseSolver::OnMontageBlendOutBegin_Networked_Internal);
	Delegate_OnMontageComplete.BindUObject(this, &UGMCE_BaseSolver::OnMontageComplete_Networked_Internal);

	Delegate_OnMontageStarted_Cosmetic.BindUObject(this, &UGMCE_BaseSolver::OnMontageStarted_Cosmetic_Internal);
	Delegate_OnMontageBlendedInEnded_Cosmetic.BindUObject(this, &UGMCE_BaseSolver::OnMontageBlendInDone_Cosmetic_Internal);
	Delegate_OnMontageBlendingOutStarted_Cosmetic.BindUObject(this, &UGMCE_BaseSolver::OnMontageBlendOutBegin_Cosmetic_Internal);
	Delegate_OnMontageEnded_Cosmetic.BindUObject(this, &UGMCE_BaseSolver::OnMontageComplete_Cosmetic_Internal);
}

void UGMCE_BaseSolver::InitializeSolver()
{
	if (bUseBlueprintEvents) BlueprintInitializeSolver();
	NativeInitializeSolver();
}

void UGMCE_BaseSolver::ActivateSolver(const FGameplayTag& ActiveMovementTag)
{
	if (bUseBlueprintEvents) BlueprintActivateSolver(ActiveMovementTag);
	NativeActivateSolver(ActiveMovementTag);
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

void UGMCE_BaseSolver::DrawDebugConnector_BP(const FVector StartPoint, const FVector EndPoint, const FLinearColor Color,
	float SphereRadius, float LineThickness)
{
	DrawDebugConnector(StartPoint, EndPoint, Color.ToFColor(true), SphereRadius, LineThickness);
}

void UGMCE_BaseSolver::DrawDebugPointNormal_BP(const FVector Point, const FVector Normal, const FLinearColor Color,
	float SphereRadius, float LineThickness)
{
	DrawDebugPointNormal(Point, Normal, Color.ToFColor(true), SphereRadius, LineThickness);
}

void UGMCE_BaseSolver::DrawDebugPointAngle_BP(const FVector Point, const FVector Direction1, const FVector Direction2,
	const FLinearColor Color, float SphereRadius, float LineThickness)
{
	DrawDebugPointAngle(Point, Direction1, Direction2, Color.ToFColor(true), SphereRadius, LineThickness);
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

void UGMCE_BaseSolver::DrawDebugSphere_BP(const FVector Origin, float SphereRadius, int Segments,
	const FLinearColor Color, float LineThickness)
{
#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	if (!bIsDebugActive) return;

	const UWorld* World = MovementComponent->GetWorld();
	DrawDebugSphere(World, Origin, SphereRadius, Segments, Color.ToFColor(true), false, -1, 0, LineThickness);
#endif
}

void UGMCE_BaseSolver::DrawDebugLine_BP(const FVector Start, const FVector End, const FLinearColor Color,
	float LineThickness)
{
#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	if (!bIsDebugActive) return;

	const UWorld* World = MovementComponent->GetWorld();
	DrawDebugLine(World, Start, End, Color.ToFColor(true), false, -1, 0, LineThickness);
#endif	
}

void UGMCE_BaseSolver::SolverLogString(const FString Message, EGMCExtendedLogType SolverLogType, bool bShowOnScreen)
{
	FColor ScreenColor = FColor::Cyan;

	FString RoleString = GetNetRoleAsString(MovementComponent->GetOwnerRole());
	if (MovementComponent->IsRemotelyControlledServerPawn())
	{
		RoleString = FString(TEXT("remote ")) + RoleString;
	}
	else if (MovementComponent->IsLocallyControlledServerPawn())
	{
		RoleString = FString(TEXT("local ")) + RoleString;
	}
	
	switch (SolverLogType)
	{
	case EGMCExtendedLogType::DebugLog:
		if (!bIsDebugActive) return;
		ScreenColor = FColor::Blue;
		UE_LOG(LogGMCExtendedSolver, Log, TEXT("[%s: %20s | %26s] %s: %s"),
			*MovementComponent->GetOwner()->GetName(), *GetNetModeAsString(MovementComponent->GetNetMode()),
			*RoleString, *GetName(), 
			*Message)
		break;
	case EGMCExtendedLogType::LogVeryVerbose:
		ScreenColor = FColor::White;
		UE_LOG(LogGMCExtendedSolver, VeryVerbose, TEXT("[%s: %20s | %26s] %s: %s"),
			*MovementComponent->GetOwner()->GetName(), *GetNetModeAsString(MovementComponent->GetNetMode()),
			*RoleString, *GetName(), 
			*Message)
		break;
	case EGMCExtendedLogType::LogVerbose:
		ScreenColor = FColor::White;
		UE_LOG(LogGMCExtendedSolver, Verbose, TEXT("[%s: %20s | %26s] %s: %s"),
			*MovementComponent->GetOwner()->GetName(), *GetNetModeAsString(MovementComponent->GetNetMode()),
			*RoleString, *GetName(), 
			*Message)
		break;
	case EGMCExtendedLogType::LogWarning:
		ScreenColor = FColor::Orange;
		UE_LOG(LogGMCExtendedSolver, Warning, TEXT("[%s: %20s | %26s] %s: %s"),
			*MovementComponent->GetOwner()->GetName(), *GetNetModeAsString(MovementComponent->GetNetMode()),
			*RoleString, *GetName(), 
			*Message)
		break;
	case EGMCExtendedLogType::LogError:
		ScreenColor = FColor::Red;
		UE_LOG(LogGMCExtendedSolver, Error, TEXT("[%s: %20s | %26s] %s: %s"),
			*MovementComponent->GetOwner()->GetName(), *GetNetModeAsString(MovementComponent->GetNetMode()),
			*RoleString, *GetName(), 
			*Message)
		break;
	}

	if (bShowOnScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, ScreenColor, Message);
	}
}

void UGMCE_BaseSolver::SolverLogText(const FText Message, EGMCExtendedLogType SolverLogType, bool bShowOnScreen)
{
	SolverLogString(Message.ToString(), SolverLogType, bShowOnScreen);
}

void UGMCE_BaseSolver::ServerSwapStateIfNeeded()
{
	if (!MovementComponent->SV_IsExecutingRemoteMoves() && MovementComponent->IsSmoothedListenServerPawn())
	{
		MovementComponent->SV_SwapServerState();
	}
}


void UGMCE_BaseSolver::NativeInitializeSolver()
{
}

void UGMCE_BaseSolver::NativeActivateSolver(const FGameplayTag& ActiveMovementTag)
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

// --- convenience functions.

bool UGMCE_BaseSolver::LineTraceSingle(const FVector Start, const FVector End, ETraceTypeQuery TraceChannel,
	bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit,
	bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::LineTraceSingle(MovementComponent, Start, End, TraceChannel,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::LineTraceMulti(const FVector Start, const FVector End, ETraceTypeQuery TraceChannel,
	bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::LineTraceMulti(MovementComponent, Start, End, TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::SphereTraceSingle(const FVector Start, const FVector End, float Radius,
                                         ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
                                         EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor,
                                         FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::SphereTraceSingle(MovementComponent, Start, End, Radius, TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::SphereTraceMulti(const FVector Start, const FVector End, float Radius,
	ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::SphereTraceMulti(MovementComponent, Start, End, Radius, TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::BoxTraceSingle(const FVector Start, const FVector End, const FVector HalfSize,
	const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::BoxTraceSingle(MovementComponent, Start, End, HalfSize, Orientation, TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::BoxTraceMulti(const FVector Start, const FVector End, FVector HalfSize,
	const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::BoxTraceMulti(MovementComponent, Start, End, HalfSize, Orientation, TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::CapsuleTraceSingle(const FVector Start, const FVector End, float Radius, float HalfHeight,
	ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::CapsuleTraceSingle(MovementComponent, Start, End, Radius, HalfHeight, TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::CapsuleTraceMulti(const FVector Start, const FVector End, float Radius, float HalfHeight,
	ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::CapsuleTraceMulti(MovementComponent, Start, End, Radius, HalfHeight, TraceChannel, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::SphereTraceSingleForObjects(const FVector Start, const FVector End, float Radius,
                                                   const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
                                                   EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor,
                                                   FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::SphereTraceSingleForObjects(MovementComponent, Start, End, Radius, ObjectTypes, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::SphereTraceMultiForObjects(const FVector Start, const FVector End, float Radius,
	const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::SphereTraceMultiForObjects(MovementComponent, Start, End, Radius, ObjectTypes, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::BoxTraceSingleForObjects(const FVector Start, const FVector End, const FVector HalfSize,
	const FRotator Orientation, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf,
	FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::BoxTraceSingleForObjects(MovementComponent, Start, End, HalfSize, Orientation, ObjectTypes,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::BoxTraceMultiForObjects(const FVector Start, const FVector End, const FVector HalfSize,
	const FRotator Orientation, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits,
	bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::BoxTraceMultiForObjects(MovementComponent, Start, End, HalfSize, Orientation, ObjectTypes,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::CapsuleTraceSingleForObjects(const FVector Start, const FVector End, float Radius,
	float HalfHeight, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf,
	FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::CapsuleTraceSingleForObjects(MovementComponent, Start, End, Radius, HalfHeight, ObjectTypes,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::CapsuleTraceMultiForObjects(const FVector Start, const FVector End, float Radius,
	float HalfHeight, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits,
	bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::CapsuleTraceMultiForObjects(MovementComponent, Start, End, Radius, HalfHeight, ObjectTypes,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::LineTraceSingleByProfile(const FVector Start, const FVector End, FName ProfileName,
	bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit,
	bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::LineTraceSingleByProfile(MovementComponent, Start, End, ProfileName,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::LineTraceMultiByProfile(const FVector Start, const FVector End, FName ProfileName,
	bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::LineTraceMultiByProfile(MovementComponent, Start, End, ProfileName, bTraceComplex,
		ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::SphereTraceSingleByProfile(const FVector Start, const FVector End, float Radius,
	FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::SphereTraceSingleByProfile(MovementComponent, Start, End, Radius, ProfileName,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::SphereTraceMultiByProfile(const FVector Start, const FVector End, float Radius,
	FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::SphereTraceMultiByProfile(MovementComponent, Start, End, Radius, ProfileName,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::BoxTraceSingleByProfile(const FVector Start, const FVector End, const FVector HalfSize,
	const FRotator Orientation, FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::BoxTraceSingleByProfile(MovementComponent, Start, End, HalfSize, Orientation,
		ProfileName, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor,
		DrawTime);
}

bool UGMCE_BaseSolver::BoxTraceMultiByProfile(const FVector Start, const FVector End, FVector HalfSize,
	const FRotator Orientation, FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::BoxTraceMultiByProfile(MovementComponent, Start, End, HalfSize, Orientation,
		ProfileName, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor,
		DrawTime);
}

bool UGMCE_BaseSolver::CapsuleTraceSingleByProfile(const FVector Start, const FVector End, float Radius,
	float HalfHeight, FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::CapsuleTraceSingleByProfile(MovementComponent, Start, End, Radius, HalfHeight, ProfileName,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::CapsuleTraceMultiByProfile(const FVector Start, const FVector End, float Radius,
	float HalfHeight, FName ProfileName, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor,
	FLinearColor TraceHitColor, float DrawTime)
{
	return UKismetSystemLibrary::CapsuleTraceMultiByProfile(MovementComponent, Start, End, Radius, HalfHeight, ProfileName,
		bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UGMCE_BaseSolver::PlayMontage(USkeletalMeshComponent* SkeletalMeshComponent, UAnimMontage* Montage,
	float StartPosition, float PlayRate, bool bBlocking, bool bRouteThroughServer)
{

	MovementComponent->SetMontageStartDelegate(Delegate_OnMontageStart, MovementComponent->MontageTracker);
	MovementComponent->SetMontageBlendInDelegate(Delegate_OnMontageBlendInComplete, MovementComponent->MontageTracker);
	MovementComponent->SetMontageBlendOutDelegate(Delegate_OnMontageBlendOutBegin, MovementComponent->MontageTracker);
	MovementComponent->SetMontageCompleteDelegate(Delegate_OnMontageComplete, MovementComponent->MontageTracker);

	UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();
	if (IsValid(AnimInstance))
	{
		AnimInstance->Montage_SetBlendedInDelegate(Delegate_OnMontageBlendedInEnded_Cosmetic, Montage);
		AnimInstance->Montage_SetBlendingOutDelegate(Delegate_OnMontageBlendingOutStarted_Cosmetic, Montage);
		AnimInstance->Montage_SetEndDelegate(Delegate_OnMontageEnded_Cosmetic, Montage);
	}

	
	if (bRouteThroughServer && !MovementComponent->GetPawnOwner()->HasAuthority())
	{
		SV_PlayMontage_Implementation(SkeletalMeshComponent, Montage, StartPosition, PlayRate, bBlocking);
	}
	
	PlayMontage_Internal(SkeletalMeshComponent, Montage, StartPosition, PlayRate, bBlocking);
	return true;
}

void UGMCE_BaseSolver::PlayMontage_Internal(USkeletalMeshComponent* SkeletalMeshComponent, UAnimMontage* Montage,
	float StartPosition, float PlayRate, bool bBlocking)
{
	if (bBlocking)
	{
		MovementComponent->PlayMontage_Blocking(SkeletalMeshComponent, MovementComponent->MontageTracker, Montage, StartPosition, PlayRate);
	}
	else
	{
		MovementComponent->PlayMontage_NonBlocking(SkeletalMeshComponent, MovementComponent->MontageTracker, Montage, StartPosition, PlayRate);
	}
}

void UGMCE_BaseSolver::SV_PlayMontage_Implementation(USkeletalMeshComponent* SkeletalMeshComponent,
                                                     UAnimMontage* Montage, float StartPosition, float PlayRate, bool bBlocking)
{
	PlayMontage_Internal(SkeletalMeshComponent, Montage, StartPosition, PlayRate, bBlocking);
}

void UGMCE_BaseSolver::OnMontageStart_Networked_Internal()
{
	SolverLogString(FString::Printf(TEXT("OnMontageStart (networked): %s"),
		*MovementComponent->MontageTracker.Montage->GetName()),
		EGMCExtendedLogType::LogVeryVerbose, false);
	OnMontageStart(MovementComponent->MontageTracker.Montage, true);
	bHasPlayedMontage = true;
}

void UGMCE_BaseSolver::OnMontageBlendInDone_Networked_Internal()
{
	SolverLogString(FString::Printf(TEXT("OnMontageBlendInDone (networked): %s"),
		*MovementComponent->MontageTracker.Montage->GetName()),
		EGMCExtendedLogType::LogVeryVerbose, false);
	OnMontageBlendInDone(MovementComponent->MontageTracker.Montage, true);
}

void UGMCE_BaseSolver::OnMontageBlendOutBegin_Networked_Internal()
{
	SolverLogString(FString::Printf(TEXT("OnMontageBlendOutBegin (networked): %s"),
		*MovementComponent->MontageTracker.Montage->GetName()),
		EGMCExtendedLogType::LogVeryVerbose, false);
	OnMontageBlendOutStarted(MovementComponent->MontageTracker.Montage, true);
}

void UGMCE_BaseSolver::OnMontageComplete_Networked_Internal()
{
	SolverLogString(FString::Printf(TEXT("OnMontageComplete (networked): %s"),
		*MovementComponent->MontageTracker.Montage->GetName()),
		EGMCExtendedLogType::LogVeryVerbose, false);
	OnMontageComplete(MovementComponent->MontageTracker.Montage, true);
	bWasInterruptedBeforeComplete = false;
	bHasPlayedMontage = false;
}

void UGMCE_BaseSolver::OnMontageStarted_Cosmetic_Internal(UAnimMontage* Montage)
{
	SolverLogString(FString::Printf(TEXT("OnMontageStart (cosmetic): %s"),
		*Montage->GetName()),
		EGMCExtendedLogType::LogVeryVerbose, false);
	OnMontageStart(Montage, false);
}

void UGMCE_BaseSolver::OnMontageBlendInDone_Cosmetic_Internal(UAnimMontage* Montage)
{
	SolverLogString(FString::Printf(TEXT("OnMontageBlendInDone (cosmetic): %s"),
		*Montage->GetName()),
		EGMCExtendedLogType::LogVeryVerbose, false);
	OnMontageBlendInDone(Montage, false);
}

void UGMCE_BaseSolver::OnMontageBlendOutBegin_Cosmetic_Internal(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		SolverLogString(FString::Printf(TEXT("OnMontageBlendOutBegin (cosmetic): %s"),
			*Montage->GetName()),
			EGMCExtendedLogType::LogVeryVerbose, false);
		OnMontageBlendOutStarted(Montage, bInterrupted);
	}
	else if (!bWasInterruptedBeforeComplete)
	{
		SolverLogString(FString::Printf(TEXT("OnMontageBlendOutBegin (cosmetic, interrupted): %s"),
			*Montage->GetName()),
			EGMCExtendedLogType::LogVeryVerbose, false);
		OnMontageInterrupted(Montage, false);
		bWasInterruptedBeforeComplete = true;
	}
}

void UGMCE_BaseSolver::OnMontageComplete_Cosmetic_Internal(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		SolverLogString(FString::Printf(TEXT("OnMontageComplete (cosmetic): %s"),
			*Montage->GetName()),
			EGMCExtendedLogType::LogVeryVerbose, false);
		OnMontageBlendOutStarted(Montage, false);
	}
	else if (!bWasInterruptedBeforeComplete)
	{
		SolverLogString(FString::Printf(TEXT("OnMontageComplete (cosmetic, interrupted): %s"),
			*Montage->GetName()),
			EGMCExtendedLogType::LogVeryVerbose, false);
		OnMontageInterrupted(Montage, false);
		bWasInterruptedBeforeComplete = true;
	}
}


void UGMCE_BaseSolver::OnMontageBlendInDone_Implementation(UAnimMontage* Montage, bool bNetworked)
{
}

void UGMCE_BaseSolver::OnMontageBlendOutStarted_Implementation(UAnimMontage* Montage, bool bNetworked)
{
}

void UGMCE_BaseSolver::OnMontageStart_Implementation(UAnimMontage* Montage, bool bNetworked)
{
}

void UGMCE_BaseSolver::OnMontageComplete_Implementation(UAnimMontage* Montage, bool bNetworked)
{
}

void UGMCE_BaseSolver::OnMontageInterrupted_Implementation(UAnimMontage* Montage, bool bNetworked)
{
}
