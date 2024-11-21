#include "Components/GMCE_OrganicMovementCmp.h"
#include "GMCExtendedLog.h"
#include "GMCE_TrackedCurveProvider.h"
#include "GMCPawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Support/GMCE_UtilityLibrary.h"
#include "Replication/Compression.h"

#define TURN_IN_PLACE_ENDPOINT 5.f

// Sets default values for this component's properties
UGMCE_OrganicMovementCmp::UGMCE_OrganicMovementCmp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGMCE_OrganicMovementCmp::BeginPlay()
{
	Super::BeginPlay();

	FString RoleString = GetNetRoleAsString(GetOwnerRole());
	if (IsRemotelyControlledServerPawn())
	{
		RoleString = FString(TEXT("remote ")) + RoleString;
	}
	else if (IsLocallyControlledServerPawn())
	{
		RoleString = FString(TEXT("local ")) + RoleString;
	}
	ComponentLogDescriptionString = RoleString;

	for (const auto& SolverClass : SolverClasses)
	{
		// We do not allow abstract parent solvers to be instantiated.
		if (SolverClass->HasAnyClassFlags(CLASS_Abstract))
			continue;
		
		// Make sure we don't have some version of this solver already instantiated, since you
		// can set up instances of solvers in configuration as well.
		for (const auto& SolverCheck : Solvers)
		{
			if (SolverCheck->GetClass()->IsChildOf(SolverClass))
				continue;
		}
		
		UGMCE_BaseSolver *NewBase = NewObject<UGMCE_BaseSolver>(this, SolverClass);
		Solvers.Add(NewBase);
	}

	// Actually set up solvers.
	for (const auto& Solver : Solvers)
	{
		Solver->SetupSolverInternal(this);
		Solver->InitializeSolver();
	}

}


// Called every frame
void UGMCE_OrganicMovementCmp::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// UpdateAllPredictions(DeltaTime);
	
	// Ragdoll nonsense.
	if (bResetMesh && IsValid(SkeletalMesh))
	{
		UPrimitiveComponent* CollisionComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
		if (IsValid(CollisionComponent))
		{
			CollisionComponent->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
			SetRootCollisionHalfHeight(PreviousCollisionHalfHeight, true, false);
			if (IsRemotelyControlledListenServerPawn())
			{
				SV_SwapServerState();
				CollisionComponent->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
				SetRootCollisionHalfHeight(PreviousCollisionHalfHeight, true, false);
				SV_SwapServerState();
			}
		}

		SkeletalMesh->SetAllBodiesSimulatePhysics(false);
		SkeletalMesh->ResetAllBodiesSimulatePhysics();
		SkeletalMesh->SetAbsolute(false, false, false);
		SkeletalMesh->AttachToComponent(GetPawnOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		SkeletalMesh->SetRelativeLocationAndRotation(PreviousRelativeMeshLocation, PreviousRelativeMeshRotation, false, nullptr, ETeleportType::ResetPhysics);

		RagdollInitialComponentOffset = FVector::ZeroVector;
		
		SetComponentToSmooth(GetSkeletalMeshReference());
		bResetMesh = false;
	}
	else if (GetMovementMode() == GetRagdollMode() && IsValid(SkeletalMesh))
	{
		if (bFirstRagdollTick)
		{
			RagdollInitialComponentOffset = SkeletalMesh->GetRelativeLocation();
			
			// Disable smoothing on simulated proxies, since it'll just make Unreal complain.
			SetComponentToSmooth(nullptr);

			SV_SwapServerState();
			PreviousCollisionHalfHeight = GetRootCollisionHalfHeight(true);
			SetRootCollisionHalfHeight(GetRootCollisionExtent(false).X, true, false);
			SV_SwapServerState();

			UPrimitiveComponent* CollisionComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
			if (IsValid(CollisionComponent))
			{
				CollisionComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
				if (IsRemotelyControlledListenServerPawn())
				{
					SV_SwapServerState();
					CollisionComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
					SV_SwapServerState();							
				}
			}
			SkeletalMesh->SetAllBodiesSimulatePhysics(true);
			SkeletalMesh->SetAllBodiesBelowLinearVelocity(RagdollBoneName, RagdollLinearVelocity, true);
			SkeletalMesh->SetAbsolute(true, false, false);

			LastRagdollBonePosition = SkeletalMesh->GetBoneLocation(RagdollBoneName);
			LastRagdollTime = GetWorld()->GetTime().GetRealTimeSeconds();

			bFirstRagdollTick = false;
		}
		else if (bShouldReplicateRagdoll && !CurrentRagdollGoal.IsZero())
		{
			FVector PelvisLocation = SkeletalMesh->GetBoneLocation(RagdollBoneName);
			FVector PelvisTarget = GetActorLocation_GMC();
			PelvisTarget.Z = PelvisLocation.Z;
			
			const FVector BoneDelta = PelvisTarget - PelvisLocation;
			const FVector PelvisOffset = SkeletalMesh->GetBoneLocation(RagdollBoneName) - SkeletalMesh->GetComponentLocation();
			const FVector ComponentTarget = PelvisTarget - PelvisOffset;

			if (!bRagdollStopped && BoneDelta.Length() > KINDA_SMALL_NUMBER)
			{
				// Figure out what needs to be done to shift the pelvis to match, if needed.
				SkeletalMesh->SetWorldLocation(ComponentTarget, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}
	
	if (bHadInput && !IsInputPresent())
	{
		InputStoppedAt = GetTime();
	}
	bHadInput = IsInputPresent();

	// Offset turn-in-place functionality.
	if (GetMovementMode() == EGMC_MovementMode::Grounded)
	{
		if (GetNetMode() != NM_Standalone && GetNetMode() != NM_DedicatedServer && TurnInPlaceType != EGMCE_TurnInPlaceType::None)
		{
			if (!FMath::IsNearlyZero(RootYawOffset, KINDA_SMALL_NUMBER) && !IsTurningInPlace())
			{
				// Smooth us back to where we should be.
				RootYawBlendTime += DeltaTime;
				if (FMath::Abs(RootYawOffset) < 2.f)
				{
					RootYawBlendTime = 1.f;
				}
				RootYawOffset = FMath::Lerp(RootYawOffset, 0.f, FMath::Clamp(RootYawBlendTime * 5.f, 0.f, 1.f));
			}

			SV_SwapServerState();
			if (!FMath::IsNearlyZero(RootYawOffset, KINDA_SMALL_NUMBER))
			{
				GetSkeletalMeshReference()->SetRelativeRotation(FRotator(0.f, RootYawOffset - 90.f, 0.f));
			}
			else
			{
				GetSkeletalMeshReference()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::ResetPhysics);
				RootYawBlendTime = 0.f;
			}			
			SV_SwapServerState();
		}
	}
	else
	{
		bTrajectoryIsPivoting = false;
		bTrajectoryIsStopping = false;
		PredictedPivotPoint = FVector::ZeroVector;
		PredictedStopPoint = FVector::ZeroVector;

#if ENABLE_DRAW_DEBUG || WITH_EDITORONLY_DATA
		bDebugHadPreviousPivot = false;
		bDebugHadPreviousStop = false;
#endif
    }

	// Debug output
#if ENABLE_DRAW_DEBUG || WITH_EDITORONLY_DATA
	if (IsTrajectoryDebugEnabled() && !IsNetMode(NM_DedicatedServer))
	{
		bool bIsPredictiveMode = GetMovementMode() == EGMC_MovementMode::Airborne || GetMovementMode() == EGMC_MovementMode::Grounded;
		
		const FVector ActorLocation = GetActorLocation_GMC();
		if (bIsPredictiveMode && !bDrawTrajectoryOnly && (bTrajectoryIsStopping || (bDebugHadPreviousStop && GetLinearVelocity_GMC().IsZero())))
		{
			if (bTrajectoryIsStopping) DebugPreviousStop = ActorLocation + PredictedStopPoint;
			DrawDebugSphere(GetWorld(), DebugPreviousStop, 24.f, 12, bTrajectoryIsStopping ? FColor::Blue : FColor::Black, false, bTrajectoryIsStopping ? -1 : 1.f, 0, bTrajectoryIsStopping ? 1.f: 2.f);
			bDebugHadPreviousStop = bTrajectoryIsStopping;
		}
		
		if (bIsPredictiveMode && !bDrawTrajectoryOnly && (bTrajectoryIsPivoting || (bDebugHadPreviousPivot && !DoInputAndVelocityDiffer())))
		{
			if (bTrajectoryIsPivoting) DebugPreviousPivot = ActorLocation + PredictedPivotPoint;
			DrawDebugSphere(GetWorld(), DebugPreviousPivot, 24.f, 12, bTrajectoryIsPivoting ? FColor::Yellow : FColor::White, false, bTrajectoryIsPivoting ? -1 : 2.f, 0, bTrajectoryIsPivoting ? 1.f : 2.f);
			bDebugHadPreviousPivot = bTrajectoryIsPivoting;
		}

		if (bIsPredictiveMode && !bDrawTrajectoryOnly && DoInputAndVelocityDiffer())
		{
			const FVector LinearVelocityDirection = GetLinearVelocity_GMC().GetSafeNormal();
			const FVector AccelerationDirection = UKismetMathLibrary::RotateAngleAxis(LinearVelocityDirection, InputVelocityOffsetAngle(), FVector(0.f, 0.f, 1.f));
			DrawDebugLine(GetWorld(), ActorLocation, ActorLocation + (AccelerationDirection * 120.f), FColor::Yellow, false, -1, 0, 2.f);
		}
		
		if (bTrajectoryEnabled)
		{
			FTransform OriginTransform;
			if (bTrajectoryUsesMesh && SkeletalMesh)
			{
				OriginTransform = SkeletalMesh->GetComponentTransform();
			}
			else
			{
				OriginTransform = UpdatedComponent->GetComponentTransform();
			}

			int PastSamples = MovementSamples.Num();
			if (bIsPredictiveMode)
			{
				PredictedTrajectory.DrawDebug(GetWorld(), OriginTransform, FColor::Blue, FColor::Green, FColor::Red, PastSamples, DebugPredictionLifeTime);
			}
			else
			{
				GetMovementHistory(false).DrawDebug(GetWorld(), OriginTransform, FColor::Blue, FColor::Green, FColor::Red, PastSamples, DebugPredictionLifeTime);
			}
		}
	}
#endif

	
}

// --- GMC Overrides
#pragma region GMC Overrides

void UGMCE_OrganicMovementCmp::BindReplicationData_Implementation()
{
	Super::BindReplicationData_Implementation();

	// Bool toggle for whether or not we have locally-input movement. This is useful
	// for animation purposes with simulated proxies.
	BI_InputPresent = BindBool(
		bInputPresent,
		EGMC_PredictionMode::ClientAuth_Input,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	// Float representing the angle by which user input differs from current velocity.
	// This is also useful for animation purposes with simulated proxies, and for
	// calculating pivot points.
	BI_InputVelocityOffset = BindSinglePrecisionFloat(
		InputVelocityOffset,
		EGMC_PredictionMode::ClientAuth_Input,
		EGMC_CombineMode::Default,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::Linear
	);

	/// Vector representing our last landing impact. This is replicated so that
	/// simulated proxies can determine whether to play 'heavy' or 'light' land
	/// animations.
	BI_LastLandingVelocity = BindCompressedVector(
		LastLandingVelocity,
		EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	// Bool representing whether we want to go into ragdoll mode or not.
	BI_WantsRagdoll = BindBool(
		bWantsRagdoll,
		EGMC_PredictionMode::ClientAuth_Input,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	// The initial linear velocity we should launch a ragdoll at.
	BI_RagdollLinearVelocity = BindCompressedVector(
		RagdollLinearVelocity,
		EGMC_PredictionMode::ClientAuth_InputOutput,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_InputOutput,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	// Should we replicate a ragdoll or not?
	BI_ShouldReplicateRagdoll = BindBool(
		bShouldReplicateRagdoll,
		EGMC_PredictionMode::ServerAuth_Input_ServerValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	// Which bone to use as the base of ragdolling.
	BI_RagdollBoneName = BindName(
		RagdollBoneName,
		EGMC_PredictionMode::ServerAuth_Input_ServerValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	// The current ragdoll 'goal' for the bone, if we're replicating
	// the ragdolling.
	BI_CurrentRagdollGoal = BindCompressedVector(
		CurrentRagdollGoal,
		EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::Linear
	);

	BI_RagdollStopped = BindBool(
		bRagdollStopped,
		EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour);

	BI_AvailableSolvers = BindGameplayTagContainer(
		AvailableSolvers,
		EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::Periodic_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	BI_CurrentActiveSolverTag = BindGameplayTag(
		CurrentActiveSolverTag,
		EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);

	BI_WantsTurnInPlace = BindBool(
		bWantsTurnInPlace,
		EGMC_PredictionMode::ClientAuth_Input,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::NearestNeighbour
	);
	
	BI_TurnInPlaceDelayedDirection = BindCompressedVector(
		TurnInPlaceDelayedDirection,
		EGMC_PredictionMode::ClientAuth_Input,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::TargetValue
	);

	BI_TurnToDirection = BindCompressedVector(
		TurnToDirection,
		EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::TargetValue
	);

	BI_PreviousMontagePosition = BindSinglePrecisionFloat(PreviousMontagePosition,
		MontageReplication.MontagePrediction.MontagePositionPredictionMode,
		MontageReplication.MontagePrediction.MontagePositionCombineMode,
		MontageReplication.MontageSimulation.bReplicateMontagePosition ? EGMC_SimulationMode::Periodic_Output : EGMC_SimulationMode::None,
		MontageReplication.MontageSimulation.MontagePositionInterpolation
	);

	if (OnBindReplicationData.IsBound())
	{
		OnBindReplicationData.Execute();
	}
}

FVector UGMCE_OrganicMovementCmp::PreProcessInputVector_Implementation(FVector InRawInputVector)
{
	FVector Result = Super::PreProcessInputVector_Implementation(InRawInputVector);
	
	if (GetMovementMode() == GetSolverMovementMode())
	{
		if (const auto ActiveSolver = GetActiveSolver())
		{
			FSolverState State = GetSolverState();
			State.ProcessedInput = Result;
			ActiveSolver->PreProcessInput(State);

			Result = State.ProcessedInput;
		}
	}
	return Result;
}

void UGMCE_OrganicMovementCmp::PreMovementUpdate_Implementation(float DeltaSeconds)
{
	Super::PreMovementUpdate_Implementation(DeltaSeconds);
	
	RunSolvers(DeltaSeconds);	
}

void UGMCE_OrganicMovementCmp::PreSimulatedMoveExecution_Implementation(const FGMC_PawnState& InputState,
	bool bCumulativeUpdate, float DeltaTime, double Timestamp)
{
	Super::PreSimulatedMoveExecution_Implementation(InputState, bCumulativeUpdate, DeltaTime, Timestamp);
}

void UGMCE_OrganicMovementCmp::MovementUpdate_Implementation(float DeltaSeconds)
{
	Super::MovementUpdate_Implementation(DeltaSeconds);

	// If we're a locally-controlled pawn (or the server), we have data on current input.
	bInputPresent = !GetProcessedInputVector().IsZero();
	InputVelocityOffset = UGMCE_UtilityLibrary::GetAngleDifferenceXY(GetLinearVelocity_GMC(), GetProcessedInputVector());
	CalculatedEffectiveAcceleration = GetTransientAcceleration();

	if (GetMovementMode() == GetSolverMovementMode())
	{
		if (const auto ActiveSolver = GetActiveSolver())
		{
			GMC_LOG(LogGMCExtended, GetOwner(), Verbose, TEXT("[%f] SOLVER:  pre-update location: %s"), GetMoveTimestamp(), *GetActorLocation_GMC().ToCompactString())
			FSolverState State = GetSolverState();
			ActiveSolver->MovementUpdate(State, DeltaSeconds);
			GMC_LOG(LogGMCExtended, GetOwner(), Verbose, TEXT("[%f] SOLVER: post-update location: %s"), GetMoveTimestamp(), *GetActorLocation_GMC().ToCompactString())
		}
	}

}

void UGMCE_OrganicMovementCmp::MovementUpdateSimulated_Implementation(float DeltaSeconds)
{
	Super::MovementUpdateSimulated_Implementation(DeltaSeconds);

	if (!IsSmoothedListenServerPawn())
	{
		if (GetMovementMode() == GetSolverMovementMode())
		{
			bool bNeedsActivate = false;
			if (CurrentActiveSolverTag != PreviousSolverTag)
			{
				OnSolverChangedMode(CurrentActiveSolverTag, PreviousSolverTag);

				UGMCE_BaseSolver* OldSolver = GetSolverForTag(PreviousSolverTag);
				UGMCE_BaseSolver* NewSolver = GetSolverForTag(CurrentActiveSolverTag);

				if (OldSolver != NewSolver)
				{
					if (OldSolver) OldSolver->DeactivateSolver();
					if (NewSolver) bNeedsActivate = true;
				}
				
				PreviousSolverTag = CurrentActiveSolverTag;
			}
		
			if (const auto ActiveSolver = GetActiveSolver())
			{
				FSolverState State = GetSolverState();
				if (bNeedsActivate) ActiveSolver->ActivateSolver(CurrentActiveSolverTag);
				ActiveSolver->MovementUpdateSimulated(State, DeltaSeconds);
			}
		}
		else if (PreviousSolverTag != FGameplayTag::EmptyTag || CurrentActiveSolverTag != FGameplayTag::EmptyTag)
		{
			if (UGMCE_BaseSolver* Solver = GetSolverForTag(PreviousSolverTag))
			{
				Solver->DeactivateSolver();
			}
			if (PreviousSolverTag != CurrentActiveSolverTag)
			{
				if (UGMCE_BaseSolver* Solver = GetSolverForTag(CurrentActiveSolverTag))
				{
					Solver->DeactivateSolver();
				}				
			}
			
			OnSolverChangedMode(FGameplayTag::EmptyTag, CurrentActiveSolverTag);
			CurrentActiveSolverTag = FGameplayTag::EmptyTag;
			PreviousSolverTag = FGameplayTag::EmptyTag;
		}		
	}
}

void UGMCE_OrganicMovementCmp::GenSimulationTick_Implementation(float DeltaTime)
{
	Super::GenSimulationTick_Implementation(DeltaTime);

	ProcessedInputVector = PreProcessInputVector(RawInputVector);
	
	// If we're being simulated, we should always synthesize an acceleration from past movements.
	UpdateAnimationHelperValues(DeltaTime);
	UpdateCalculatedEffectiveAcceleration();

	if (GetMovementMode() == GetSolverMovementMode())
	{
		if (UGMCE_BaseSolver* Solver = GetActiveSolver())
		{
			auto State = GetSolverState();
			Solver->SimulationTick(State, DeltaTime);
		}
	}
	
	if (!IsSmoothedListenServerPawn())
	{
		UpdateAllPredictions(DeltaTime);
	}
	
	UpdateTurnInPlaceState(true);

	if (bWantsTurnInPlace && (IsTurningInPlace() || TurnInPlaceState == EGMCE_TurnInPlaceState::Starting))
	{
		ApplyTurnInPlace(DeltaTime, true);
	}

	if (TurnInPlaceState == EGMCE_TurnInPlaceState::Running && !bWantsTurnInPlace && !IsSmoothedListenServerPawn())
	{
		EndTurnInPlace(true);
	}
}

void UGMCE_OrganicMovementCmp::GenPredictionTick_Implementation(float DeltaTime)
{
	Super::GenPredictionTick_Implementation(DeltaTime);
	UpdateAnimationHelperValues(DeltaTime);

	if (GetMovementMode() == GetSolverMovementMode())
	{
		if (UGMCE_BaseSolver* Solver = GetActiveSolver())
		{
			auto State = GetSolverState();
			Solver->PredictionTick(State, DeltaTime);
		}
	}
}

void UGMCE_OrganicMovementCmp::GenAncillaryTick_Implementation(float DeltaTime, bool bLocalMove,
                                                               bool bCombinedClientMove)
{
	Super::GenAncillaryTick_Implementation(DeltaTime, bLocalMove, bCombinedClientMove);

	if (GetMovementMode() == GetSolverMovementMode())
	{
		if (UGMCE_BaseSolver* Solver = GetActiveSolver())
		{
			auto State = GetSolverState();
			Solver->AncillaryTick(State, DeltaTime);
		}
	}
	
	UpdateAllPredictions(DeltaTime);
}

bool UGMCE_OrganicMovementCmp::UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds)
{
	if (bWantsRagdoll || (GetMovementMode() == GetRagdollMode()))
	{
		// We may need to either enable or disable ragdoll mode.
		if (bWantsRagdoll && (GetMovementMode() == EGMC_MovementMode::Grounded || GetMovementMode() == EGMC_MovementMode::Airborne))
		{
			RagdollLinearVelocity = GetRagdollInitialVelocity();
			HaltMovement();
			SetMovementMode(GetRagdollMode());
		}
		else if (!bWantsRagdoll)
		{
			SetMovementMode(EGMC_MovementMode::Airborne);
		}
		return true;
	}

	if (CurrentActiveSolverTag.IsValid())
	{
		SetMovementMode(GetSolverMovementMode());
		return true;
	}
	
	// If we disable solver mode, we revert to airborne to allow GMC to sort things out itself.
	if (GetMovementMode() == GetSolverMovementMode())
	{
		LeaveSolverMode();
	}
	
	return Super::UpdateMovementModeDynamic_Implementation(Floor, DeltaSeconds);
}

void UGMCE_OrganicMovementCmp::OnMovementModeChanged_Implementation(EGMC_MovementMode PreviousMovementMode)
{
	if (GetMovementMode() == GetRagdollMode())
	{
		SetRagdollActive(true);
	}
	else if (PreviousMovementMode == GetRagdollMode())
	{
		SetRagdollActive(false);
	}

	if (GetMovementMode() != GetSolverMovementMode() && PreviousMovementMode == GetSolverMovementMode())
	{
		if (UGMCE_BaseSolver* Solver = GetActiveSolver())
		{
			Solver->DeactivateSolver();
		}
		
		OnSolverChangedMode(FGameplayTag::EmptyTag, CurrentActiveSolverTag);
		PreviousSolverTag = FGameplayTag::EmptyTag;
		CurrentActiveSolverTag = FGameplayTag::EmptyTag;
	}
	
	Super::OnMovementModeChanged_Implementation(PreviousMovementMode);
}

void UGMCE_OrganicMovementCmp::OnMovementModeChangedSimulated_Implementation(EGMC_MovementMode PreviousMovementMode)
{
	if (GetMovementMode() == GetRagdollMode())
	{
		SetRagdollActive(true);
	}
	else if (PreviousMovementMode == GetRagdollMode())
	{
		SetRagdollActive(false);
	}

	if (GetMovementMode() == GetSolverMovementMode() && !IsSmoothedListenServerPawn())
	{
		bool bNeedsActivate = CurrentActiveSolverTag != PreviousSolverTag;

		if (bNeedsActivate)
		{
			if (auto Solver = GetSolverForTag(PreviousSolverTag))
			{
				Solver->DeactivateSolver();
			}
			
			if (auto Solver = GetActiveSolver())
			{
				Solver->ActivateSolver(GetActiveSolverTag());
			}
		}
		PreviousSolverTag = CurrentActiveSolverTag;
	}
	else if (GetMovementMode() != GetSolverMovementMode() && PreviousMovementMode == GetSolverMovementMode())
	{
		if (UGMCE_BaseSolver* Solver = GetSolverForTag(PreviousSolverTag))
		{
			Solver->DeactivateSolver();
		}
		
		OnSolverChangedMode(FGameplayTag::EmptyTag, CurrentActiveSolverTag);
		CurrentActiveSolverTag = FGameplayTag::EmptyTag;
		PreviousSolverTag = FGameplayTag::EmptyTag;
	}

	Super::OnMovementModeChangedSimulated_Implementation(PreviousMovementMode);
}

void UGMCE_OrganicMovementCmp::PostMovementUpdate_Implementation(float DeltaSeconds)
{
	Super::PostMovementUpdate_Implementation(DeltaSeconds);

	if (GetMovementMode() == GetSolverMovementMode() && GetActiveSolver())
	{
		FSolverState State = GetSolverState();
		GetActiveSolver()->PostMovementUpdate(State, DeltaSeconds);
	}
}

void UGMCE_OrganicMovementCmp::PostSimulatedMoveExecution_Implementation(const FGMC_PawnState& OutputState,
	bool bCumulativeUpdate, float DeltaTime, double Timestamp)
{
	Super::PostSimulatedMoveExecution_Implementation(OutputState, bCumulativeUpdate, DeltaTime, Timestamp);

	if (GetMovementMode() == GetSolverMovementMode() && GetActiveSolver())
	{
		FSolverState State = GetSolverState();
		GetActiveSolver()->PostMovementUpdateSimulated(State, DeltaTime);
	}
}

float UGMCE_OrganicMovementCmp::GetMaxSpeed() const
{
	// If strafe speed limits are disabled or we're not on the ground, just stick with our normal value.
	if (!bLimitStrafeSpeed || !IsMovingOnGround() ||
		StrafeSpeedPoints.Num() <= 1 || GetLinearVelocity_GMC().IsNearlyZero()) return Super::GetMaxSpeed();

	float StrafeAngle = FMath::Abs(GetLocomotionAngle());

	float LimitedMaxSpeed = MaxDesiredSpeed;
	float CurrentStartSpeed = MaxDesiredSpeed;
	float MinAngle = 0.f;

	if (StrafeAngle >= StrafeSpeedPoints.Last().AngleOffset)
	{
		// We're past the last breakpoint, not worth iterating.
		LimitedMaxSpeed = MaxDesiredSpeed * StrafeSpeedPoints.Last().SpeedFactor;
	}
	else
	{
		for (int Idx = 0; Idx < StrafeSpeedPoints.Num(); Idx++)
		{
			const float MaxAngle = StrafeSpeedPoints[Idx].AngleOffset;
			
			if (StrafeAngle >= MinAngle && StrafeAngle <= MaxAngle)
			{
				// This is our breakpoint.
				const float Range = MaxAngle - MinAngle;
				const float Position = (StrafeAngle - MinAngle) / Range;

				LimitedMaxSpeed = FMath::Lerp(CurrentStartSpeed, MaxDesiredSpeed * StrafeSpeedPoints[Idx].SpeedFactor, Position);
				break;
			}

			CurrentStartSpeed = MaxDesiredSpeed * StrafeSpeedPoints[Idx].SpeedFactor;
			MinAngle = MaxAngle;
		}
	}
	
	// If the analog input is less than our max speed anyway, use the lower value.
	return FMath::Min(LimitedMaxSpeed, Super::GetMaxSpeed());
}

void UGMCE_OrganicMovementCmp::PhysicsCustom_Implementation(float DeltaSeconds)
{
	if (GetMovementMode() == GetRagdollMode() && bShouldReplicateRagdoll && IsValid(SkeletalMesh))
	{
		const bool bIsBoneAuthority = IsRagdollBoneAuthority();
		
		const FVector BoneLocation = SkeletalMesh->GetBoneLocation(RagdollBoneName);
		const FVector BoneVelocity = SkeletalMesh->GetBoneLinearVelocity(RagdollBoneName) * FVector(1.f, 1.f, 0.f);
		
		if (bIsBoneAuthority)
		{
			FVector BoneOffset = BoneLocation - CurrentRagdollGoal;
			
			// Set our goal for clients to use.
			bRagdollStopped = BoneOffset.Length() <= 5.f;
			CurrentRagdollGoal = BoneLocation;
		}

		if (!CurrentRagdollGoal.IsZero())
		{
			const FVector RagdollBoneDelta = CurrentRagdollGoal - BoneLocation;
		
			if (!IsSimulatedProxy() && (!BoneVelocity.IsNearlyZero() || RagdollBoneDelta.Size() > 2.f))
			{
				FVector NewLocation = CurrentRagdollGoal;
				NewLocation.Z = UpdatedComponent->GetComponentLocation().Z;
				FCollisionQueryParams CollisionParameters = FCollisionQueryParams(NAME_None, true, GetOwner());
				CollisionParameters.AddIgnoredComponent(UpdatedPrimitive);
				
				FHitResult GroundHit;
				const FVector StartCheck = CurrentRagdollGoal + FVector(0.f, 0.f, PreviousCollisionHalfHeight);
				const FVector EndCheck = CurrentRagdollGoal - FVector(0.f, 0.f, 25.f);
				GetWorld()->LineTraceSingleByChannel(GroundHit, StartCheck, EndCheck, ECC_Pawn, CollisionParameters);

				if (GroundHit.bBlockingHit)
				{
					NewLocation.Z = FMath::Min(UpdatedComponent->GetComponentLocation().Z, GroundHit.ImpactPoint.Z + PreviousCollisionHalfHeight);
				}
				
				// Move our character to stay with the pelvis. We do this on the client, too, to make the
				// overall effect smooth.
				const FVector Delta = NewLocation - UpdatedComponent->GetComponentLocation();
		
				if (Delta.Size() > KINDA_SMALL_NUMBER)
				{
					FHitResult GroundResult;
					SafeMoveUpdatedComponent(NewLocation - UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat(), false, GroundResult, ETeleportType::TeleportPhysics);
				}
			}
		}

		return;
	}

	if (GetMovementMode() == GetSolverMovementMode())
	{
		if (const auto Solver = GetActiveSolver())
		{
			FSolverState State = GetSolverState();

			GMC_LOG(LogGMCExtended, GetOwner(), Verbose, TEXT("[%f] SOLVER:  pre-physics location: %s"), GetMoveTimestamp(), *GetActorLocation_GMC().ToCompactString())
			if (Solver->PerformMovement(State, DeltaSeconds))
			{
				// Solver wants to maintain control. However, make sure to check if our active solver tag needs to
				// change. This may be true for something like the climbing solver, which handles both climbing
				// and mantling.
				if (const FGameplayTag& NewSolverTag = Solver->GetPreferredSolverTag(); NewSolverTag != CurrentActiveSolverTag)
				{
					GMC_LOG(LogGMCExtended, GetOwner(), Verbose, TEXT("[%f] SOLVER: %s changed active movement tag from %s to %s"),
						GetMoveTimestamp(), *Solver->GetName(), *CurrentActiveSolverTag.ToString(), *NewSolverTag.ToString())
					
					// Solver is changing into some different sub-mode.
					PreviousSolverTag = CurrentActiveSolverTag;
					OnSolverChangedMode(NewSolverTag, CurrentActiveSolverTag);
					CurrentActiveSolverTag = NewSolverTag;
					Solver->ActivateSolver(NewSolverTag);
				}
			}
			else
			{
				GMC_LOG(LogGMCExtended, GetOwner(), Verbose, TEXT("[%f] SOLVER: %s surrendered control of movement"),
					GetMoveTimestamp(), *Solver->GetName())

				Solver->DeactivateSolver();
				
				OnSolverChangedMode(FGameplayTag::EmptyTag, CurrentActiveSolverTag);
				PreviousSolverTag = CurrentActiveSolverTag;
				CurrentActiveSolverTag = FGameplayTag::EmptyTag;
			}
		}
		else
		{
			// No valid solver available. Bail.
			LeaveSolverMode();
		}
		return;
	}
	
	Super::PhysicsCustom_Implementation(DeltaSeconds);
}

float UGMCE_OrganicMovementCmp::GetInputAccelerationCustom_Implementation() const
{
	if (GetMovementMode() == GetRagdollMode())
	{
		return 0.f;
	}
	
	return Super::GetInputAccelerationCustom_Implementation();
}

void UGMCE_OrganicMovementCmp::CalculateVelocity(float DeltaSeconds)
{
	if (IsMovingOnGround())
	{
		// If we're using "Require Facing Before Move" and we're on the ground and we're not currently
		// moving, we want to check the direction we're TRYING to face and see if we're offset at all.
		if (ShouldTurnInPlace() && Velocity.IsNearlyZero())
		{
			if (bOrientToControlRotationDirection && TurnInPlaceAngleThreshold > 0.f)
			{
				UpdateTurnInPlaceState();
				FVector ControlDirection = GetControllerRotation_GMC().Vector();
				const float ControlAngle = FMath::Abs(UGMCE_UtilityLibrary::GetAngleDifferenceXY(ControlDirection, UpdatedComponent->GetForwardVector()));
				if ((TurnInPlaceAngleThreshold > 0.f && ControlAngle > TurnInPlaceAngleThreshold) || IsTurningInPlace() || TurnInPlaceState == EGMCE_TurnInPlaceState::Starting)
				{
					Velocity = FVector::ZeroVector;
					CalculateTurnInPlace(DeltaSeconds);
					return;
				}
			}
			else if (!bOrientToControlRotationDirection && FacingAngleOffsetThreshold > 0.f)
			{
				FVector InputDirection = GetProcessedInputVector();
				bool bFinishTurn = false;
				if (InputDirection.IsNearlyZero())
				{
					InputDirection = TurnToDirection;
					bFinishTurn = true;
				}
				const float VelocityAngle = FMath::Abs(UGMCE_UtilityLibrary::GetAngleDifferenceXY(InputDirection, UpdatedComponent->GetForwardVector()));

				// If our velocity is less than our threshold angle, we can move normally. Otherwise, we'll want to just
				// turn-in-place instead.
				if (VelocityAngle > FacingAngleOffsetThreshold || bFinishTurn)
				{
					// We're not calling our parent implementation, and we're not moving until we're within our angle threshold.
					Velocity = FVector::ZeroVector;
					TurnToDirection = (VelocityAngle < FacingAngleOffsetThreshold) ? FVector::ZeroVector : InputDirection;
		
					if (bUseSafeRotations)
					{
						RotateYawTowardsDirectionSafe(InputDirection, RotationRate, DeltaSeconds);
					}
					else
					{
						RotateYawTowardsDirection(InputDirection, RotationRate, DeltaSeconds);
					}
					return;
				}
			}
		}
		else if (bLockVelocityToRotationRate && !Velocity.IsNearlyZero())
		{
			// If we are locking velocity to rotation rate, we want to constrain how quickly our velocity will change.
			const FRotator CurrentRotation = RoundRotator(UpdatedComponent->GetComponentRotation(), EGMC_FloatPrecisionBlueprint::TwoDecimals);
			const FRotator TargetRotation = RoundRotator(UKismetMathLibrary::Conv_VectorToRotator(Velocity), EGMC_FloatPrecisionBlueprint::TwoDecimals);
			const float CurrentYaw = FRotator::NormalizeAxis(CurrentRotation.Yaw);
			const float TargetYaw = FRotator::NormalizeAxis(TargetRotation.Yaw);

			if (!FMath::IsNearlyEqual(CurrentYaw, TargetYaw, 0.01))
			{
				// Our yaw differs enough that we should check it.
				const float MaxDeltaYaw = FRotator::NormalizeAxis(RotationRate * DeltaSeconds);
				if (MaxDeltaYaw < FMath::Abs(TargetYaw - CurrentYaw))
				{
					// Our yaw differs enough that we're exceeding our rotation rate.
					const float NewRotationYaw = FMath::FixedTurn(CurrentYaw, TargetYaw, MaxDeltaYaw);
					const FRotator NewRotation = FRotator(0.f, NewRotationYaw, 0.f);

					// Generate new input and velocity, constrained by our rotation rate.
					const FVector NewDirection = UKismetMathLibrary::Conv_RotatorToVector(NewRotation).GetSafeNormal2D();
					Velocity = Velocity.Size2D() * NewDirection;
					ProcessedInputVector = ProcessedInputVector.Size2D() * NewDirection;
				}
			}
		}
	}

	Super::CalculateVelocity(DeltaSeconds);

	if (!Velocity.IsNearlyZero())
	{
		TurnInPlaceSecondsAccumulated = 0.f;
		TurnInPlaceDelayedDirection = FVector::ZeroVector;
	}
}

void UGMCE_OrganicMovementCmp::ApplyDirectionalInput(const FInputActionInstance& InputAction)
{
	Super::ApplyDirectionalInput(InputAction);
}

void UGMCE_OrganicMovementCmp::OnLanded_Implementation(const FVector& ImpactVelocity)
{
	Super::OnLanded_Implementation(ImpactVelocity);

	// Store our impact velocity for purposes of animation.
	LastLandingVelocity = ImpactVelocity;
}

void UGMCE_OrganicMovementCmp::RotateYawTowardsDirection(const FVector& Direction, float Rate, float DeltaTime)
{
	Super::RotateYawTowardsDirection(Direction, Rate, DeltaTime);

	CalculateAimYawRemaining(Direction);
}

bool UGMCE_OrganicMovementCmp::RotateYawTowardsDirectionSafe(const FVector& Direction, float Rate, float DeltaTime)
{
	const bool bResult = Super::RotateYawTowardsDirectionSafe(Direction, Rate, DeltaTime);

	CalculateAimYawRemaining(Direction);
	
	return bResult;
}

UPrimitiveComponent* UGMCE_OrganicMovementCmp::FindActorBase_Implementation()
{
	if (GetMovementMode() == GetSolverMovementMode() && CurrentActiveSolver)
	{
		FSolverState State = GetSolverState();
		SolverAppliedBase = CurrentActiveSolver->GetSolverBase(State);
		if (SolverAppliedBase) return SolverAppliedBase;
	}
	return Super::FindActorBase_Implementation();
}

void UGMCE_OrganicMovementCmp::ApplyRotation(bool bIsDirectBotMove,
                                             const FGMC_RootMotionVelocitySettings& RootMotionMetaData, float DeltaSeconds)
{
	if (bIsDirectBotMove)
	{
		Super::ApplyRotation(bIsDirectBotMove, RootMotionMetaData, DeltaSeconds);
		return;
	}
	
	// If we're orienting to velocity and either have no root motion or are applying rotation atop
	// root motion, rotate towards the direction we're moving in.
	if (bOrientToVelocityDirection && (!HasRootMotion() || RootMotionMetaData.bApplyRotationWithRootMotion))
	{
		// We default towards orienting towards our velocity.
		FVector OrientTowards = Velocity;

		// If we aren't moving and orient to input direction is *also* set, we'll utilize input direction
		// until we're actually moving. This works well with bRequireFacingBeforeMove.
		if (bOrientToInputDirection && GetSpeedXY() < MinimumVelocityForOrientation)
		{
			OrientTowards = GetProcessedInputVector();
		}
		
		if(bUseSafeRotations)
		{
			RotateYawTowardsDirectionSafe(OrientTowards, RotationRate, DeltaSeconds);
		}
		else
		{
			RotateYawTowardsDirection(OrientTowards, RotationRate, DeltaSeconds);
		}
		return;
	}

	if (ShouldTurnInPlace() && GetOwnerRole() != ROLE_SimulatedProxy && (IsTurningInPlace() || TurnInPlaceState == EGMCE_TurnInPlaceState::Starting || (bOrientToControlRotationDirection && Velocity.IsNearlyZero() && (!HasRootMotion() || RootMotionMetaData.bApplyRotationWithRootMotion))))
	{
		CalculateTurnInPlace(DeltaSeconds);
		return;
	}

	// Otherwise just let GMC handle it as normal.
	Super::ApplyRotation(bIsDirectBotMove, RootMotionMetaData, DeltaSeconds);
}

void UGMCE_OrganicMovementCmp::MontageUpdate(float DeltaSeconds)
{
	PreviousMontagePosition = MontageTracker.MontagePosition;
	Super::MontageUpdate(DeltaSeconds);
}

void UGMCE_OrganicMovementCmp::OnMontageStarted(UAnimMontage* Montage, float Position, float PlayRate,
                                                bool bInterrupted, float MontageDelta, float DeltaSeconds)
{
	PreviousMontagePosition = Position;
	Super::OnMontageStarted(Montage, Position, PlayRate, bInterrupted, MontageDelta, DeltaSeconds);
}

FString UGMCE_OrganicMovementCmp::GetComponentDescription() const
{
	if (ComponentLogDescriptionString.Len() > 0)
	{
		return ComponentLogDescriptionString;
	}
	
	return FString(TEXT("unknown"));
}

#pragma endregion 

#pragma region Animation Support

void UGMCE_OrganicMovementCmp::PreProcessRootMotion(const FGMC_AnimMontageInstance& MontageInstance,
	FRootMotionMovementParams& InOutRootMotionParams, float MontageDelta, float DeltaSeconds)
{
	// If we've got a bound delegate to handle modifying root motion, call it. This is used by GMCExAnim to
	// handle motion warping.
	if (ProcessRootMotionPreConvertToWorld.IsBound())
	{
		bool bUsePredictedIfPresent = GetOwner()->GetNetMode() != NM_Standalone;
		const FTransform MeshRelativeTransform = SkeletalMesh->GetRelativeTransform();
		const FTransform WarpedRootMotionTransform = ProcessRootMotionPreConvertToWorld.Execute(InOutRootMotionParams.GetRootMotionTransform(), GetActorTransform_GMC(), MeshRelativeTransform, this, DeltaSeconds, bUsePredictedIfPresent);

		InOutRootMotionParams.Set(WarpedRootMotionTransform);
	}

	Super::PreProcessRootMotion(MontageInstance, InOutRootMotionParams, MontageDelta, DeltaSeconds);
}

void UGMCE_OrganicMovementCmp::OnSyncDataApplied_Implementation(const FGMC_PawnState& State, EGMC_NetContext Context)
{
	Super::OnSyncDataApplied_Implementation(State, Context);

	if (OnSyncDataAppliedDelegate.IsBound())
	{
		OnSyncDataAppliedDelegate.Execute(State, Context);
	}
}

void UGMCE_OrganicMovementCmp::UpdateAnimationHelperValues(float DeltaSeconds)
{
	LastAimRotation = CurrentAimRotation;
	CurrentAimRotation = GetControllerRotation_GMC().GetNormalized();
	CurrentAimYawRate = (CurrentAimRotation.Yaw - LastAimRotation.Yaw) / DeltaSeconds;

	LastComponentRotation = CurrentComponentRotation;
	CurrentComponentRotation = UpdatedComponent->GetComponentRotation().GetNormalized();
	CurrentComponentYawRate = (LastComponentRotation.Yaw - CurrentComponentRotation.Yaw) / DeltaSeconds;

	CurrentAnimationAcceleration = (Velocity - LastAnimationVelocity) / DeltaSeconds;
	LastAnimationVelocity = Velocity;
}

void UGMCE_OrganicMovementCmp::CalculateAimYawRemaining(const FVector& DirectionVector)
{
	if (DirectionVector.IsNearlyZero())
	{
		AimYawRemaining = 0.f;
		return;
	}

	AimYawRemaining = UKismetMathLibrary::FindRelativeLookAtRotation(GetActorTransform(), GetActorLocation_GMC() + DirectionVector).Yaw;
}

float UGMCE_OrganicMovementCmp::GetMaxPredictionSpeed(const FVector& InputVector)
{
	if (bIgnoreInputModifier)
	{
		// Modifier is being ignored.
		return MaxDesiredSpeed;
	}

	// Calculate the modified max speed.
	gmc_ck(MaxDesiredSpeed >= MinAnalogWalkSpeed)
	return FMath::Clamp(InputVector.Length() * MaxDesiredSpeed, MinAnalogWalkSpeed, BIG_NUMBER);	
}

#pragma endregion


bool UGMCE_OrganicMovementCmp::IsInputPresent(bool bAllowGrace) const
{
	if (IsSimulatedProxy() && bAllowGrace)
	{
		return bInputPresent || (GetTime() - InputStoppedAt < 0.01);
	}

	return bInputPresent;
}

void UGMCE_OrganicMovementCmp::UpdateAllPredictions(float DeltaTime)
{
	if (bTrajectoryEnabled)
	{
		// Track trajectory even when we're in custom movement modes.
		UpdateMovementSamples();
	}
	
	if (GetMovementMode() == EGMC_MovementMode::Grounded || GetMovementMode() == EGMC_MovementMode::Airborne)
	{
		if (GetMovementMode() == EGMC_MovementMode::Grounded)
		{
			if (bPrecalculateDistanceMatches)
			{
				UpdateStopPrediction(DeltaTime);
				UpdatePivotPrediction(DeltaTime);
				UpdateStartPrediction(DeltaTime);
			}
		}

		if (bTrajectoryEnabled && bPrecalculateFutureTrajectory)
		{
			// Only predict trajectory when we're grounded or in airborne mode.
			UpdateTrajectoryPrediction();
		}
	}
}

void UGMCE_OrganicMovementCmp::UpdateCalculatedEffectiveAcceleration()
{
	if (GetLinearVelocity_GMC().IsZero())
	{
		CalculatedEffectiveAcceleration = FVector::ZeroVector;
	}
	
	const int32 Idx = GetCurrentMoveHistoryNum();
	if (Idx < 1) return;

	const double SyncedTime = GetTime();
	
	const FGMC_Move LastMove = AccessMoveHistory(Idx - 1);
	if (!LastMove.HasValidTimestamp() || LastMove.MetaData.Timestamp == SyncedTime) return;

	const FVector DeltaV = GetLinearVelocity_GMC() - GetLinearVelocityFromState(LastMove.OutputState);
	CalculatedEffectiveAcceleration = -DeltaV / (SyncedTime - LastMove.MetaData.Timestamp);
}

void UGMCE_OrganicMovementCmp::UpdateStopPrediction(float DeltaTime)
{
	PredictedStopPoint = PredictGroundedStopLocation(GetLinearVelocity_GMC(), GetBrakingDeceleration(), GroundFriction, DeltaTime);
	bTrajectoryIsStopping = !PredictedStopPoint.IsZero() && !IsInputPresent();	
}

void UGMCE_OrganicMovementCmp::UpdatePivotPrediction(float DeltaTime)
{
	const FRotator Rotation = GetActorRotation_GMC();

	PredictedPivotPoint = PredictGroundedPivotLocation(GetCurrentEffectiveAcceleration(), GetLinearVelocity_GMC(), Rotation, GroundFriction, DeltaTime, FMath::Clamp(PivotPredictionAngleThreshold, 30.f, 179.f));
	bTrajectoryIsPivoting = !PredictedPivotPoint.IsZero() && IsInputPresent() && DoInputAndVelocityDiffer();	
}

void UGMCE_OrganicMovementCmp::UpdateStartPrediction(float DeltaTime)
{
	if (GetCurrentAnimationAcceleration().IsZero() && LastStartVelocityCheck.IsZero() && !bTrajectoryIsPivoting && !bLastStoppedPivotCheck)
	{
		bTrajectoryIsStarting = false;
		bLastStoppedPivotCheck = false;
		LastStoppedTimestamp = GetTime();
		return;
	}

	bTrajectoryIsStarting = (GetTime() - LastStoppedTimestamp < 0.2) && !bTrajectoryIsPivoting && !bLastStoppedPivotCheck;
	bLastStoppedPivotCheck = bTrajectoryIsPivoting;
	LastStartVelocityCheck = GetLinearVelocity_GMC();

}

bool UGMCE_OrganicMovementCmp::IsStopPredicted(FVector& OutStopPrediction) const
{
	OutStopPrediction = PredictedStopPoint;
	return bTrajectoryIsStopping;
}

bool UGMCE_OrganicMovementCmp::IsPivotPredicted(FVector& OutPivotPrediction) const
{
	OutPivotPrediction = PredictedPivotPoint;
	return bTrajectoryIsPivoting;
}

FVector UGMCE_OrganicMovementCmp::PredictGroundedStopLocation(const FVector& CurrentVelocity, float BrakingDeceleration,
	float Friction, float DeltaTime)
{
	if (CurrentVelocity.IsZero())
	{
		return FVector::ZeroVector;
	}
	
	FVector Result = FVector::ZeroVector;
	
	const FVector GroundedVelocity = CurrentVelocity * FVector(1.f, 1.f, 0.f);
	const FVector GroundedDirection = GroundedVelocity.GetSafeNormal();
	
	if (const float RealBrakingDeceleration = BrakingDeceleration * Friction; RealBrakingDeceleration > 0.f)
	{
		Result = GroundedDirection * (GroundedVelocity.SizeSquared() / (2 * RealBrakingDeceleration));
	}
	
	return Result;
}

FVector UGMCE_OrganicMovementCmp::PredictGroundedPivotLocation(const FVector& CurrentAcceleration,
	const FVector& CurrentVelocity, const FRotator& CurrentRotation, float Friction, float DeltaTime, float AngleThreshold)
{
	FVector Result = FVector::ZeroVector;
	const FVector EffectiveAcceleration = CurrentAcceleration;
	
	const FVector GroundedVelocity = CurrentVelocity * FVector(1.f, 1.f, 0.f);
	const FVector LocalVelocity = UKismetMathLibrary::LessLess_VectorRotator(GroundedVelocity, CurrentRotation);
	const FVector LocalAcceleration = UKismetMathLibrary::LessLess_VectorRotator(EffectiveAcceleration, CurrentRotation);

	const FVector Acceleration2D = EffectiveAcceleration * FVector(1.f, 1.f, 0.f);
	FVector AccelerationDir2D;
	float AccelerationSize2D;
	Acceleration2D.ToDirectionAndLength(AccelerationDir2D, AccelerationSize2D);

	const float VelocityAlongAcceleration = (GroundedVelocity | AccelerationDir2D);
	const float DotProduct = UKismetMathLibrary::Dot_VectorVector(LocalVelocity, LocalAcceleration);
	const float AngleOffset = FMath::Abs(UGMCE_UtilityLibrary::GetAngleDifferenceXY(CurrentVelocity, CurrentAcceleration));
	
	if (AngleOffset >= AngleThreshold && VelocityAlongAcceleration < 0.f)
	{
		const float SpeedAlongAcceleration = -VelocityAlongAcceleration;
		const float Divisor = AccelerationSize2D + 2.f * SpeedAlongAcceleration * Friction;
		const float TimeToDirectionChange = SpeedAlongAcceleration / Divisor;

		const FVector AccelerationForce = EffectiveAcceleration -
			(GroundedVelocity - AccelerationDir2D * GroundedVelocity.Size2D()) * Friction;

		Result = GroundedVelocity * TimeToDirectionChange + 0.5f * AccelerationForce *
			TimeToDirectionChange * TimeToDirectionChange;
	}

	return Result;	
}

FGMCE_MovementSampleCollection UGMCE_OrganicMovementCmp::GetMovementHistory(bool bOmitLatest) const
{
	FGMCE_MovementSampleCollection Result;

	const int32 TrajectoryHistoryCount = MovementSamples.Num();
	if (TrajectoryHistoryCount > 0)
	{
		Result.Samples.Reserve(TrajectoryHistoryCount);

		int32 Counter = 0;
		for (auto& Sample : MovementSamples)
		{
			Counter++;
			if (!bOmitLatest || Counter != TrajectoryHistoryCount - 1)
				Result.Samples.Emplace(Sample);
		}
	}

	return Result;	
}

FGMCE_MovementSampleCollection UGMCE_OrganicMovementCmp::PredictMovementFuture(const FTransform& FromOrigin,
	const FRotator& ControllerRotation, const FQuat& MeshOffset, bool bIncludeHistory)
{
	const float TimePerSample = 1.f / TrajectorySimSampleRate;
	const int32 TotalSimulatedSamples = FMath::TruncToInt32(TrajectorySimSampleRate * TrajectorySimSeconds);

	const int32 TotalCollectionSize = TotalSimulatedSamples + 1 + (bIncludeHistory ? MovementSamples.Num() : 0);
	
	FGMCE_MovementSample SimulatedSample = LastMovementSample;
	FGMCE_MovementSampleCollection Predictions;
	Predictions.Samples.Reserve(TotalCollectionSize);

	if (bIncludeHistory)
	{
		Predictions.Samples.Append(GetMovementHistory(false).Samples);
	}

	FVector PredictedAcceleration;
	FRotator RotationVelocity;
	FVector TempVector;
	
	GetCurrentAccelerationRotationVelocityFromHistory(PredictedAcceleration, RotationVelocity, EGMCE_TrajectoryRotationType::Component);
	RotationVelocity = FRotator(0.f, FMath::Clamp(RotationVelocity.Yaw, -RotationRate, RotationRate), 0.f);
	FRotator RotationVelocityPerSample = RotationVelocity * TimePerSample;

	FRotator TravelRotation;
	GetCurrentAccelerationRotationVelocityFromHistory(TempVector, TravelRotation, EGMCE_TrajectoryRotationType::Travel);	
	
	FRotator ControllerRotationVelocity;
	GetCurrentAccelerationRotationVelocityFromHistory(TempVector, ControllerRotationVelocity, EGMCE_TrajectoryRotationType::Controller);
	FRotator ControllerRotationVelocityPerSample = FRotator(0.f, ControllerRotationVelocity.Yaw, 0.f) * TimePerSample;

	FRotator MeshOffsetRotationVelocity;
	GetCurrentAccelerationRotationVelocityFromHistory(TempVector, MeshOffsetRotationVelocity, EGMCE_TrajectoryRotationType::MeshOffset);
	FRotator MeshOffsetRotationVelocityPerSample = FRotator(0.f, MeshOffsetRotationVelocity.Yaw, 0.f) * TimePerSample;

	EGMC_MovementMode EffectiveMovementMode = GetMovementMode();
	const FVector InputVector = PreProcessInputVector(RawInputVector);
	PredictedAcceleration = InputVector * GetInputAcceleration();
	PredictedAcceleration.Z = 0.f;

	FRotator RotationToUse;
	if (bTrajectoryUsesControllerRotation)
	{
		RotationToUse = ControllerRotationVelocity;
	}
	else
	{
		RotationToUse = TravelRotation;
	}
	
	RotationToUse.Yaw = FMath::Min(RotationToUse.Yaw, RotationRate);
	FRotator AccelerationRotation = RotationToUse * TimePerSample;
	
	float BrakingDeceleration = GetBrakingDeceleration();
	float BrakingFriction = IsAirborne() ? 1.f : GetGroundFriction();
	float MaxSpeed = GetMaxPredictionSpeed(InputVector);

	const bool bZeroFriction = BrakingFriction == 0.f;
	const bool bNoBrakes = BrakingDeceleration == 0.f;
	
	FVector CurrentLocation = FromOrigin.GetLocation();
	FRotator CurrentRotation = FromOrigin.GetRotation().Rotator();
	FRotator CurrentControllerRotation = ControllerRotation;
	FRotator CurrentMeshOffsetRotation = MeshOffset.Rotator();
	FVector CurrentVelocity = GetLinearVelocity_GMC();

	const FVector InitialAccel = PredictedAcceleration;
	const FVector InitialVel = CurrentVelocity;
	FVector Deceleration;
	float DistanceTraveled = 0.f;
	int SampleCount = 0;

	for (int32 Idx = 0; Idx < TotalSimulatedSamples; Idx++)
	{
		if (!RotationVelocityPerSample.IsNearlyZero())
		{
			CurrentRotation += RotationVelocityPerSample;
			RotationVelocityPerSample.Yaw /= 1.1f;
		}
		if (!ControllerRotationVelocityPerSample.IsNearlyZero())
		{
			CurrentControllerRotation += ControllerRotationVelocityPerSample;
			ControllerRotationVelocityPerSample.Yaw /= 1.1f;
		}
		if (!CurrentMeshOffsetRotation.IsNearlyZero())
		{
			CurrentMeshOffsetRotation += MeshOffsetRotationVelocityPerSample;
			MeshOffsetRotationVelocityPerSample.Yaw /= 1.1f;
		}

		if (EffectiveMovementMode == EGMC_MovementMode::Airborne)
		{
			// *singing a'la Idina Menzel* I guess I'm not... defying gravity.. 
			CurrentVelocity += GetGravity() * TimePerSample;
		}
		
		const float DecelerationScale = FMath::Clamp(1.f - (PredictedAcceleration.GetSafeNormal() | CurrentVelocity.GetSafeNormal()), 0.f, 1.f);
		Deceleration = GetBrakingDeceleration() * DecelerationScale * -CurrentVelocity.GetSafeNormal() * BrakingFriction;

		if (EffectiveMovementMode == EGMC_MovementMode::Airborne && Deceleration.Z < 0.f)
		{
			// Don't decelerate against gravity...
			Deceleration.Z = 0.f;
		}

		FVector PreviousAcceleration = PredictedAcceleration;
		if (!Deceleration.IsZero() && (EffectiveMovementMode == EGMC_MovementMode::Airborne || !IsInputPresent()))
		{
			Deceleration = ClampToMinDeceleration(Deceleration);
			PredictedAcceleration += Deceleration;
		}

		if (DirectionsDifferXY(PreviousAcceleration, PredictedAcceleration))
		{
			PredictedAcceleration = FVector(0.f, 0.f, PredictedAcceleration.Z);
		}
		if (DirectionsDifferZ(PreviousAcceleration, PredictedAcceleration))
		{
			PredictedAcceleration = FVector(PredictedAcceleration.X, PredictedAcceleration.Y, 0.f);
		}
		
		const FVector PreviousVelocity = CurrentVelocity;
		
		if (PredictedAcceleration.IsNearlyZero())
		{
			if (!CurrentVelocity.IsNearlyZero())
			{
				constexpr float MaxPredictedTrajectoryTimeStep = 1.f / 33.f;

				float RemainingTime = TimePerSample;
				while (RemainingTime >= 1e-6f && !CurrentVelocity.IsZero())
				{
					const float dt = RemainingTime > MaxTimeStep && !bZeroFriction ?
						FMath::Min(MaxPredictedTrajectoryTimeStep, RemainingTime * 0.5f) : RemainingTime;
					RemainingTime -= dt;

					CurrentVelocity = CurrentVelocity + (Deceleration * dt);
					if ((CurrentVelocity | PreviousVelocity) < 0.f)
					{
						CurrentVelocity = FVector::ZeroVector;
						break;
					}
				}

				if (CurrentVelocity.SizeSquared() < KINDA_SMALL_NUMBER)
				{
					CurrentVelocity = FVector::ZeroVector;
				}
			}
			else
			{
				CurrentVelocity = FVector::ZeroVector;
			}
		}
		else
		{
			const FVector AccelerationDirection = PredictedAcceleration.GetSafeNormal();
			const float Speed = CurrentVelocity.Size();

			CurrentVelocity = CurrentVelocity - (CurrentVelocity - AccelerationDirection * Speed) *
				(BrakingFriction * TimePerSample);

			CurrentVelocity += PredictedAcceleration * TimePerSample;

			CurrentVelocity = CurrentVelocity.GetClampedToMaxSize2D(MaxSpeed);
		}

		if (EffectiveMovementMode == EGMC_MovementMode::Grounded && CurrentVelocity.Z < MaxGroundedVelocityZ)
		{
			// Stick to the ground if we're not exceeding the max grounded velocity.
			// If we're predicting collisions anyway, we'll adapt to a slope if needed.
			CurrentVelocity.Z = 0.f;
		}

		const FVector PreviousLocation = CurrentLocation;
		CurrentLocation += CurrentVelocity * TimePerSample;
		bool bUseAsMark = false;

		const float PredictedDrop = CurrentVelocity.Z * TimePerSample;

		if (bTrajectoryPredictCollisions)
		{
			FVector Start = CurrentLocation + FVector::UpVector * GetMaxStepUpHeight();
			FVector End = CurrentLocation - FVector::UpVector * (EffectiveMovementMode == EGMC_MovementMode::Grounded ? GetMaxStepDownHeight() : PredictedDrop);

			FHitResult Hit;
			UKismetSystemLibrary::LineTraceSingle(this, Start, End,
				UEngineTypes::ConvertToTraceType(ECC_Visibility), false, { },
				EDrawDebugTrace::None, Hit, true);
			
			if (!Hit.bStartPenetrating)
			{
				if (Hit.bBlockingHit && (FMath::Abs(Hit.Location.Z - CurrentLocation.Z) > 2.f))
				{
					CurrentLocation = Hit.Location;
					// CurrentVelocity.Z = (CurrentLocation.Z - PreviousLocation.Z) / TimePerSample;
				}

				if (EffectiveMovementMode == EGMC_MovementMode::Grounded && !Hit.bBlockingHit)
				{
					// Handle transition to falling.
					EffectiveMovementMode = EGMC_MovementMode::Airborne;
					PredictedAcceleration = FVector::ZeroVector;
					BrakingDeceleration = BrakingDecelerationAirborne;
					BrakingFriction = 1.f;
					bUseAsMark = true;
				}
			
				if (EffectiveMovementMode == EGMC_MovementMode::Airborne && Hit.bBlockingHit)
				{
					// Handle transition to ground
					EffectiveMovementMode = EGMC_MovementMode::Grounded;
					CurrentVelocity.Z = 0.f;
					PredictedAcceleration = InitialAccel;
					PredictedAcceleration = PredictedAcceleration.GetClampedToMaxSize(GetInputAcceleration());
					PredictedAcceleration.Z = 0.f;
					BrakingDeceleration = BrakingDecelerationGrounded;
					BrakingFriction = GetGroundFriction();
					bUseAsMark = true;
				}				
			}
		}

		const FTransform NewTransform = FTransform(CurrentRotation.Quaternion(), CurrentLocation);
		const FTransform NewRelativeTransform = NewTransform.GetRelativeTransform(FromOrigin);

		const FRotator FacingRotation = NewTransform.GetRotation().Rotator() - MeshOffset.Rotator();

		const FTransform ActorTransform = FTransform(FacingRotation, CurrentLocation);

		DistanceTraveled += (CurrentLocation - PreviousLocation).Size();
		
		SimulatedSample = FGMCE_MovementSample();
		SimulatedSample.RelativeTransform = NewRelativeTransform;
		SimulatedSample.RelativeLinearVelocity = FromOrigin.InverseTransformVectorNoScale(CurrentVelocity);
		SimulatedSample.WorldTransform = NewTransform;
		SimulatedSample.WorldLinearVelocity = CurrentVelocity;
		SimulatedSample.AccumulatedSeconds = TimePerSample * (Idx + 1);
		SimulatedSample.ControllerRotation = CurrentControllerRotation;
		SimulatedSample.MeshComponentRelativeRotation = CurrentMeshOffsetRotation.Quaternion();
		SimulatedSample.ActorWorldTransform = ActorTransform;
		SimulatedSample.ActorWorldRotation = ActorTransform.GetRotation().Rotator();
		SimulatedSample.Acceleration = PredictedAcceleration;
		SimulatedSample.bUseAsMarker = bUseAsMark;

		Predictions.Samples.Emplace(SimulatedSample);

		SampleCount++;

		if (bTrajectoryUsesControllerRotation && bTrajectoryStopsAtControllerRotation)
		{
			float ControllerYawDelta = FMath::Abs(FRotator::NormalizeAxis(GetControllerRotation_GMC().Yaw) - FRotator::NormalizeAxis(ControllerRotation.Yaw));
			if (ControllerYawDelta <= 0.5f)
			{
				AccelerationRotation = FRotator::ZeroRotator;
				ControllerRotationVelocityPerSample = FRotator::ZeroRotator;
			}
		}
		
		// Rotate acceleration at the end, so we can handle deceleration in a sane fashion.
		if (!AccelerationRotation.IsNearlyZero())
		{
			PredictedAcceleration = AccelerationRotation.RotateVector(PredictedAcceleration);
			if (!bTrajectoryUsesControllerRotation || !bTrajectoryStopsAtControllerRotation)
			{
				// If we use controller rotation, we stop when we're near our current rotation.
				// If we're using velocity rotation, we need to decay our rotation a bit.
				AccelerationRotation.Yaw /= TrajectoryRotationDecay ? TrajectoryRotationDecay : 1.1f;
			}
		}
	}

	return Predictions;	
}

void UGMCE_OrganicMovementCmp::UpdateTrajectoryPrediction()
{
	FTransform OriginTransform;
	FQuat MeshOffsetRotation;
	if (bTrajectoryUsesMesh && IsValid(SkeletalMesh))
	{
		OriginTransform = SkeletalMesh->GetComponentTransform();
		MeshOffsetRotation = SkeletalMesh->GetRelativeRotation().Quaternion();
	}
	else
	{
		OriginTransform = UpdatedComponent->GetComponentTransform();
		MeshOffsetRotation = FQuat::Identity;
	}
	PredictedTrajectory = PredictMovementFuture(OriginTransform, FRotator(0.f, GetControllerRotation_GMC().Yaw, 0.f), MeshOffsetRotation, true);	
}

FGMCE_MovementSample UGMCE_OrganicMovementCmp::GetMovementSampleFromCurrentState() const
{
	// FTransform CurrentTransform = GetPawnOwner()->GetActorTransform();
	// const FVector CurrentLocation = GetLowerBound();
	// CurrentTransform.SetLocation(CurrentLocation);

	FTransform CurrentTransform;
	FQuat MeshRelativeRotation = FQuat::Identity;
	if (bTrajectoryUsesMesh && IsValid(SkeletalMesh))
	{
		CurrentTransform = SkeletalMesh->GetComponentTransform();
		MeshRelativeRotation = SkeletalMesh->GetRelativeRotation().Quaternion();
	}
	else
	{
		if (bTrajectoryUsesMesh)
		{
			// We are only here if Skeletal Mesh is null.
			UE_LOG(LogGMCExtended, Warning, TEXT("%s has no skeletal mesh but bTrajectoryUsesMesh is true. Reverting to using collision capsule for trajectory."), *GetName())
		}
		CurrentTransform = GetPawnOwner()->GetActorTransform();
		const FVector CurrentLocation = GetLowerBound();
		CurrentTransform.SetLocation(CurrentLocation);
	}

	FGMCE_MovementSample Result = FGMCE_MovementSample(CurrentTransform, GetLinearVelocity_GMC());
	Result.ActorWorldTransform = GetPawnOwner()->GetActorTransform();
	Result.ActorWorldRotation = GetPawnOwner()->GetActorRotation();
	Result.MeshComponentRelativeRotation = MeshRelativeRotation;
	Result.ControllerRotation = FRotator(0.f, GetControllerRotation_GMC().Yaw, 0.f);
	Result.Acceleration = GetProcessedInputVector();
	if (!LastMovementSample.IsZeroSample())
	{
		Result.ActorDeltaRotation = Result.ActorWorldRotation - LastMovementSample.ActorWorldRotation;
	}

	return Result;
}

void UGMCE_OrganicMovementCmp::AddNewMovementSample(const FGMCE_MovementSample& Sample)
{
	const float GameSeconds = UKismetSystemLibrary::GetGameTimeInSeconds(GetWorld());
	if (LastTrajectoryGameSeconds != 0.f)
	{
		const float DeltaSeconds = GameSeconds - LastTrajectoryGameSeconds;
		const float DeltaDistance = Sample.DistanceFrom(LastMovementSample);
		const FTransform DeltaTransform = LastMovementSample.WorldTransform.GetRelativeTransform(Sample.WorldTransform);

		for (auto& OldSample : MovementSamples)
		{
			OldSample.PrependRelativeOffset(DeltaTransform, -DeltaSeconds);
		}

		CullMovementSampleHistory(FMath::IsNearlyZero(DeltaDistance), Sample);
	}

	MovementSamples.Emplace(Sample);
	LastMovementSample = Sample;
	LastTrajectoryGameSeconds = GameSeconds;		
}

void UGMCE_OrganicMovementCmp::CullMovementSampleHistory(bool bIsNearlyZero, const FGMCE_MovementSample& LatestSample)
{
	const FGMCE_MovementSample FirstSample = MovementSamples.First();
	const float FirstSampleTime = FirstSample.AccumulatedSeconds;

	if (bIsNearlyZero && !FirstSample.IsZeroSample())
	{
		// We were moving, and stopped.
		if (EffectiveTrajectoryTimeDomain == 0.f)
		{
			// Initialize a time horizon while we're motionless, to allow uniform decay to continue.
			EffectiveTrajectoryTimeDomain = FirstSampleTime;
		}
	}
	else
	{
		// We're moving again, so we no longer need a horizon.
		EffectiveTrajectoryTimeDomain = 0.f;
	}

	MovementSamples.RemoveAll([&](const FGMCE_MovementSample& TestSample)
	{
		if (TestSample.IsZeroSample() && LatestSample.IsZeroSample())
		{
			// We don't need duplicate zero motion samples, they just clutter the history.
			return true;
		}

		if (TestSample.AccumulatedSeconds < -TrajectoryHistorySeconds)
		{
			// Sample is too old for the buffer.
			return true;
		}

		if (EffectiveTrajectoryTimeDomain != 0.f && (TestSample.AccumulatedSeconds < EffectiveTrajectoryTimeDomain))
		{
			// Time horizon is in effect, prune everything before our zero motion moment.
			return true;
		}

		// Keep the sample
		return false;
	});		
}

void UGMCE_OrganicMovementCmp::UpdateMovementSamples_Implementation()
{
	const float GameSeconds = UKismetSystemLibrary::GetGameTimeInSeconds(GetWorld());
	const float Period = TrajectoryHistoryPeriod ? TrajectoryHistoryPeriod : SMALL_NUMBER;
	
	if (GameSeconds - LastTrajectoryGameSeconds > Period)
	{
		AddNewMovementSample(GetMovementSampleFromCurrentState());
	}		
}

void UGMCE_OrganicMovementCmp::GetCurrentAccelerationRotationVelocityFromHistory(FVector& OutAcceleration,
	FRotator& OutRotationVelocity, const EGMCE_TrajectoryRotationType& RotationType) const
{
	const auto HistoryArray = MovementSamples;
	if (HistoryArray.IsEmpty())
	{
		OutAcceleration = FVector::ZeroVector;
		OutRotationVelocity = FRotator::ZeroRotator;
		return;
	}
	
	for (int32 Idx = HistoryArray.Num() - 1; Idx >= 0; Idx--)
	{
		const FGMCE_MovementSample Sample = HistoryArray[Idx];

		if (LastMovementSample.AccumulatedSeconds - Sample.AccumulatedSeconds >= 0.1f)
		{
			OutRotationVelocity = LastMovementSample.GetRotationVelocityFrom(Sample, RotationType);
			OutAcceleration = LastMovementSample.GetAccelerationFrom(Sample);
			return;
		}
	}

	OutRotationVelocity = FRotator::ZeroRotator;
	OutAcceleration = FVector::ZeroVector;
}

FVector UGMCE_OrganicMovementCmp::GetCurrentVelocityFromHistory()
{
	const auto HistoryArray = MovementSamples;
	if (HistoryArray.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	
	for (int32 Idx = HistoryArray.Num() - 1; Idx >= 0; Idx--)
	{
		const FGMCE_MovementSample Sample = HistoryArray[Idx];
		float TimeDelta = LastMovementSample.AccumulatedSeconds - Sample.AccumulatedSeconds;
		
		if (TimeDelta >= 0.1f)
		{
			return (LastMovementSample.WorldTransform.GetLocation() - Sample.WorldTransform.GetLocation()) / TimeDelta;
		}
	}

	return FVector::ZeroVector;
}

void UGMCE_OrganicMovementCmp::EnableRagdoll()
{
	bWantsRagdoll = true;
}

void UGMCE_OrganicMovementCmp::DisableRagdoll()
{
	bWantsRagdoll = false;
}

FVector UGMCE_OrganicMovementCmp::GetRagdollInitialVelocity_Implementation()
{
	return GetLinearVelocity_GMC();
}

void UGMCE_OrganicMovementCmp::SetRagdollActive(bool bActive)
{
	if (IsNetMode(NM_DedicatedServer) && !bShouldReplicateRagdoll) return;
	
	if (bActive)
	{
		if (PreviousRelativeMeshLocation.IsZero())
		{
			PreviousRelativeMeshLocation = SkeletalMesh->GetRelativeLocation();
			PreviousRelativeMeshRotation = SkeletalMesh->GetRelativeRotation();
		}
		HaltMovement();
	}
	else
	{
		CurrentRagdollGoal = FVector::ZeroVector;
		bRagdollStopped = true;
	}

	bEnablePhysicsInteraction = !bActive;
	bFirstRagdollTick = bActive;
	bResetMesh = !bActive;
}

bool UGMCE_OrganicMovementCmp::IsRagdollBoneAuthority() const
{
	// return GetNetMode() == NM_Standalone || GetOwnerRole() == ROLE_AutonomousProxy || IsLocallyControlledListenServerPawn();

	return GetNetMode() == NM_Standalone || GetOwnerRole() == ROLE_Authority;
}

void UGMCE_OrganicMovementCmp::RunSolvers(float DeltaTime)
{
	FSolverState State = GetSolverState();
	State.AvailableSolvers.Reset();
	
	for (const auto& Solver : Solvers)
	{
		Solver->RunSolver(State, DeltaTime);
	}

	AvailableSolvers = State.AvailableSolvers;	
}

bool UGMCE_OrganicMovementCmp::ShouldDebugSolver(const FGameplayTag& SolverTag) const
{
#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	return DebugSolvers.HasTag(SolverTag);
#else
	return false;
#endif
}

bool UGMCE_OrganicMovementCmp::IsSolutionAvailableForSolver(FGameplayTag SolverTag) const
{
	return AvailableSolvers.HasTag(SolverTag);	
}

UGMCE_BaseSolver* UGMCE_OrganicMovementCmp::GetActiveSolver()
{
	if (GetMovementMode() != GetSolverMovementMode() || !CurrentActiveSolverTag.IsValid())
	{
		CurrentActiveSolver = nullptr;
		return nullptr;
	}

	if (!CurrentActiveSolver || !CurrentActiveSolverTag.MatchesTag(CurrentActiveSolver->GetTag()))
	{
		// Refresh our cached solver.
		for (const auto& Solver : Solvers)
		{
			if (CurrentActiveSolverTag.MatchesTag(Solver->GetTag()))
			{
				CurrentActiveSolver = Solver;
				break;
			}
		}
	}

	return CurrentActiveSolver;	
}

UGMCE_BaseSolver* UGMCE_OrganicMovementCmp::GetSolverForTag(FGameplayTag SolverTag) const
{
	if (!SolverTag.IsValid()) return nullptr;

	for (const auto& Solver : Solvers)
	{
		if (SolverTag.MatchesTag(Solver->GetTag()))
		{
			return Solver;
		}
	}

	return nullptr;	
}

bool UGMCE_OrganicMovementCmp::TryActivateSolver(const FGameplayTag& SolverTag)
{
	if (!SolverTag.IsValid())
	{
		OnSolverChangedMode(FGameplayTag::EmptyTag, CurrentActiveSolverTag);
		CurrentActiveSolverTag = FGameplayTag::EmptyTag;
		PreviousSolverTag = FGameplayTag::EmptyTag;
		CurrentActiveSolver = nullptr;
		return true;
	}
	
	if (AvailableSolvers.HasTag(SolverTag))
	{
		for (const auto& Solver : Solvers)
		{
			if (SolverTag.MatchesTag(Solver->GetTag()))
			{
				GMC_LOG(LogGMCExtended, GetOwner(), Verbose, TEXT("[%f] SOLVER: trying to activate solver %s for tag %s"),
					GetMoveTimestamp(), *Solver->GetName(), *SolverTag.ToString())
				
				PreviousSolverTag = CurrentActiveSolverTag;
				CurrentActiveSolverTag = Solver->GetPreferredSolverTag();
				OnSolverChangedMode(CurrentActiveSolverTag, PreviousSolverTag);
				Solver->ActivateSolver(CurrentActiveSolverTag);
				return true;
			}
		}
	}

	return false;	
}

FSolverState UGMCE_OrganicMovementCmp::GetSolverState() const
{
	FSolverState State;
	State.Location = GetActorLocation_GMC();
	State.Rotation = GetActorRotation_GMC();
	State.LinearVelocity = GetLinearVelocity_GMC();
	State.MovementMode = GetMovementMode();
	State.MovementTag = CurrentActiveSolverTag;
	State.RawInput = RawInputVector;
	State.ProcessedInput = ProcessedInputVector;
	State.AvailableSolvers = AvailableSolvers;

	return State;	
}

void UGMCE_OrganicMovementCmp::LeaveSolverMode()
{
	FHitResult HitCheck;
	FCollisionQueryParams CollisionQueryParams(NAME_None, false, GetOwner());
	CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
	GetWorld()->LineTraceSingleByChannel(HitCheck, GetLowerBound() + FVector(0.f, 0.f, MaxStepUpHeight),
										 GetLowerBound() - FVector(0.f, 0.f, MaxStepDownHeight),
										 ECC_Pawn, FCollisionQueryParams(NAME_None, true, GetOwner()));
	if (HitWalkableFloor(HitCheck))
	{
		SetMovementMode(EGMC_MovementMode::Grounded);
	}
	else
	{
		SetMovementMode(EGMC_MovementMode::Airborne);
	}
}


void UGMCE_OrganicMovementCmp::UpdateTurnInPlaceState(bool bSimulated)
{
	switch (TurnInPlaceState)
	{
	case EGMCE_TurnInPlaceState::Ready:
		if (bWantsTurnInPlace && !(IsSmoothedListenServerPawn() && bSimulated))
		{
			TurnInPlaceState = EGMCE_TurnInPlaceState::Starting;
		}
		break;
	case EGMCE_TurnInPlaceState::Starting:
	case EGMCE_TurnInPlaceState::Running:
		break;
	case EGMCE_TurnInPlaceState::Done:
		if (!bWantsTurnInPlace)
		{
			TurnInPlaceState = EGMCE_TurnInPlaceState::Ready;
		}
		break;
	}
}

void UGMCE_OrganicMovementCmp::SetStrafingMovement(bool bStrafingEnabled)
{
	bOrientToVelocityDirection = !bStrafingEnabled;
	bOrientToInputDirection = !bStrafingEnabled;
	bOrientToControlRotationDirection = bStrafingEnabled;
}

bool UGMCE_OrganicMovementCmp::ShouldTurnInPlace() const
{
	// Turn in place is disabled.
	if (TurnInPlaceType == EGMCE_TurnInPlaceType::None) return false;

	// If we have root motion present, allow it to drive us rather than trying to turn in place.
	if (HasRootMotion()) return false;

	// Turn in place is driven by movement component but has an instant rotation rate.
	if (TurnInPlaceType == EGMCE_TurnInPlaceType::MovementComponent && TurnInPlaceRotationRate <= 0.f) return false;

	// We're in "Orient to Control Rotation Direction" mode but have no angle or delay set.
	if (bOrientToControlRotationDirection && (TurnInPlaceAngleThreshold <= 0.f && TurnInPlaceDelay <= 0.f)) return false;

	// We're in "Require Facing Before Move" mode and our facing angle offset threshold is invalid.
	if (bRequireFacingBeforeMove && FacingAngleOffsetThreshold <= 0.f) return false;

	// Hey, let's turn in place.
	return true;
}

void UGMCE_OrganicMovementCmp::CalculateTurnInPlace(float DeltaSeconds)
{
	// We want the visual values for this, as this is used for animation.
	SV_SwapServerState();
	const FVector ControllerForward = UKismetMathLibrary::Conv_RotatorToVector(GetControllerRotation_GMC());
	const float ControllerAngle = FMath::Abs(UGMCE_UtilityLibrary::GetAngleDifferenceXY(ControllerForward, UpdatedComponent->GetForwardVector()));
	SV_SwapServerState();

	if (!bWantsTurnInPlace && TurnInPlaceState == EGMCE_TurnInPlaceState::Ready && ControllerAngle >= FacingAngleOffsetThreshold &&
		(GetOwnerRole() == ROLE_Authority || GetOwnerRole() == ROLE_AutonomousProxy || GetNetMode() == NM_Standalone))
	{
		// We aren't (yet) turning in place, but we are past our threshold angle.
		
		if (GetCurrentAimYawRate() < UE_KINDA_SMALL_NUMBER)
		{
			// We're no longer rotating, so we can start accumulating our delay.
			TurnInPlaceSecondsAccumulated += DeltaSeconds;
		}
		else
		{
			// If we're still spinning the controller, reset our accumulated delay.
			TurnInPlaceSecondsAccumulated = 0.f;
		}
			
		if ((TurnInPlaceDelay > 0.f && TurnInPlaceSecondsAccumulated >= TurnInPlaceDelay) || (ControllerAngle >= TurnInPlaceAngleThreshold) || IsInputPresent())
		{
			SV_SwapServerState();
			// We have exceeded our turn-in-place delay, or input is now present.
			// Either way, record our start and end points and let's get this show on the road.
			bWantsTurnInPlace = true;
			TurnInPlaceDelayedDirection = GetControllerRotation_GMC().Vector().GetSafeNormal2D();
			SV_SwapServerState();
		}
	}
	else if (TurnInPlaceState == EGMCE_TurnInPlaceState::Running || TurnInPlaceState == EGMCE_TurnInPlaceState::Starting)
	{
		ApplyTurnInPlace(DeltaSeconds, false);
		if (FMath::Abs(ComponentYawRemaining) < TURN_IN_PLACE_ENDPOINT || (!bWantsTurnInPlace && FMath::Abs(ComponentYawRemaining) < 15.f))
		{
			EndTurnInPlace();
		}
	}
}

void UGMCE_OrganicMovementCmp::ApplyTurnInPlace(float DeltaSeconds, bool bSimulated)
{
	if (TurnInPlaceState == EGMCE_TurnInPlaceState::Done || TurnInPlaceState == EGMCE_TurnInPlaceState::Ready) return;

	if (TurnInPlaceState == EGMCE_TurnInPlaceState::Starting)
	{
		if (!IsSmoothedListenServerPawn() || bSimulated)
		{
			TurnInPlaceStartDirection = UpdatedComponent->GetForwardVector();
			TurnInPlaceTotalYaw = ComponentYawRemaining = UGMCE_UtilityLibrary::GetAngleDifferenceXY(UpdatedComponent->GetForwardVector(), TurnInPlaceDelayedDirection);
			TurnInPlaceLastYaw = 0.f;
			TurnInPlaceState = EGMCE_TurnInPlaceState::Running;
		}
	}
	
	if (GetOwnerRole() == ROLE_SimulatedProxy && TurnInPlaceType == EGMCE_TurnInPlaceType::MovementComponent)
	{
		// We always at least want a component yaw remaining value for our simulated proxy.
		ComponentYawRemaining = UGMCE_UtilityLibrary::GetAngleDifferenceXY(UpdatedComponent->GetForwardVector(), TurnInPlaceDelayedDirection);
		return;
	}

	bool bHasValidCurve = false;
	float CurveValue = 0.f;

	const UAnimInstance* AnimInstance = GetSkeletalMeshReference()->GetAnimInstance();
	if (TurnInPlaceType == EGMCE_TurnInPlaceType::TrackedCurveValue && IsValid(AnimInstance))
	{
		const IGMCE_TrackedCurveProvider* CurveProvider = Cast<IGMCE_TrackedCurveProvider>(AnimInstance);
		if (CurveProvider != nullptr)
		{
			bHasValidCurve = true;
			CurveValue = CurveProvider->GetTrackedCurve(FName(TEXT("TurnInPlace")));
		}
	}
	
	// We only handle rotations if we're on an autonomous proxy or the authority, and for the authority we cannot
	// be the simulation cycle of a smoothed listen server pawn. Standalone rotation is also handled separately.
	if (!bHasValidCurve || (GetOwnerRole() != ROLE_SimulatedProxy && GetNetMode() != NM_Standalone && (!IsSmoothedListenServerPawn() || !bSimulated)))
	{
		// Either we're using movement component logic, OR we didn't have a valid curve we were supposed to use.
		if (bUseSafeRotations)
		{
			RotateYawTowardsDirectionSafe(TurnInPlaceDelayedDirection, TurnInPlaceRotationRate, DeltaSeconds);				
		}
		else
		{
			RotateYawTowardsDirection(TurnInPlaceDelayedDirection, TurnInPlaceRotationRate, DeltaSeconds);
		}
	}
	
	if (bHasValidCurve)
	{
		// We have a valid curve. Determine how we use the curve.
		float YawChange = 0.f;

		if (TurnInPlaceType == EGMCE_TurnInPlaceType::TrackedCurveValue)
		{
			YawChange = CurveValue - TurnInPlaceLastYaw;
			TurnInPlaceLastYaw = CurveValue;
		}

		if (GetNetMode() == NM_Standalone)
		{
			// If we're standalone, utilize the curve value directly. Easy.
			const FRotator DeltaRotation = FRotator( 0.f, YawChange, 0.f);
			UpdatedComponent->AddLocalRotation(DeltaRotation);
		}
		else
		{
			// No point on messing with the mesh component on a dedicated server, or our non-simulated side
			// of a smoothed pawn.
			if (GetNetMode() != NM_DedicatedServer && TurnInPlaceState == EGMCE_TurnInPlaceState::Running && !(IsSmoothedListenServerPawn() && !bSimulated))
			{
				// Offset our skeletal mesh component to keep the animation smooth.
				const float YawOffsetFromStart = UGMCE_UtilityLibrary::GetAngleDifferenceXY(TurnInPlaceStartDirection, UpdatedComponent->GetForwardVector());
				RootYawOffset = (CurveValue - YawOffsetFromStart);
				// if (TurnInPlaceTotalYaw < 0.f)
				// {
				// 	RootYawOffset = FMath::Clamp(RootYawOffset, TurnInPlaceTotalYaw, 0.f);
				// }
				// else
				// {
				// 	RootYawOffset = FMath::Clamp(RootYawOffset, 0.f, TurnInPlaceTotalYaw);					
				// }
			}
		}
	}

	if (!IsSmoothedListenServerPawn() || bSimulated)
	{
		ComponentYawRemaining = UGMCE_UtilityLibrary::GetAngleDifferenceXY(UpdatedComponent->GetForwardVector(), TurnInPlaceDelayedDirection);
	}

}

void UGMCE_OrganicMovementCmp::EndTurnInPlace(bool bSimulated)
{
	if (TurnInPlaceState != EGMCE_TurnInPlaceState::Running) return;
	
	TurnInPlaceSecondsAccumulated = 0.f;
	TurnInPlaceStartDirection = FVector::ZeroVector;
	TurnInPlaceDelayedDirection = FVector::ZeroVector;
	TurnInPlaceState = EGMCE_TurnInPlaceState::Done;
	if (!IsSimulatedProxy()) bWantsTurnInPlace = false;	// Rely on replication to reset?
}

float UGMCE_OrganicMovementCmp::GetTurnInPlaceDuration() const
{
	if (TurnInPlaceState == EGMCE_TurnInPlaceState::Done || TurnInPlaceState == EGMCE_TurnInPlaceState::Ready || TurnInPlaceRotationRate == 0.f || TurnInPlaceType == EGMCE_TurnInPlaceType::None) return 0.f;

	const FRotator Rounded = RoundRotator(FRotator(0.f, ComponentYawRemaining, 0.f), EGMC_FloatPrecisionBlueprint::TwoDecimals);
	const float Result = (FMath::Abs(Rounded.Yaw) / TurnInPlaceRotationRate) * (GetNetMode() == NM_Standalone ? 2.f : 1.8f); // I wish I knew why 1.8 was a magic number that made this work smoothly.

	return Result;
}


float UGMCE_OrganicMovementCmp::CalculateDirection(const FVector& Direction, const FRotator& BaseRotation)
{
	if (!Direction.IsNearlyZero())
	{
		const FMatrix RotMatrix = FRotationMatrix(BaseRotation);
		const FVector ForwardVector = RotMatrix.GetScaledAxis(EAxis::X);
		const FVector RightVector = RotMatrix.GetScaledAxis(EAxis::Y);
		const FVector NormalizedVel = Direction.GetSafeNormal2D();

		// get a cos(alpha) of forward vector vs velocity
		const float ForwardCosAngle = static_cast<float>(FVector::DotProduct(ForwardVector, NormalizedVel));
		// now get the alpha and convert to degree
		float ForwardDeltaDegree = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));

		// depending on where right vector is, flip it
		const float RightCosAngle = static_cast<float>(FVector::DotProduct(RightVector, NormalizedVel));
		if (RightCosAngle < 0.f)
		{
			ForwardDeltaDegree *= -1.f;
		}

		return ForwardDeltaDegree;
	}

	return 0.f;
}

float UGMCE_OrganicMovementCmp::GetLocomotionAngle() const
{
	return CalculateDirection(GetLinearVelocity_GMC(), GetCurrentComponentRotation());
}

float UGMCE_OrganicMovementCmp::GetOrientationAngle() const
{
	float Angle = GetLocomotionAngle();

	if (FMath::Abs(Angle) > 90.f)
	{
		Angle += 180.f;
		if (Angle > 180.f)
		{
			Angle -= 360.f;
		}
	}

	return Angle;
}

void UGMCE_OrganicMovementCmp::EnableTrajectoryDebug(bool bEnabled)
{
#if ENABLE_DRAW_DEBUG || WITH_EDITORONLY_DATA
	bDrawDebugPredictions = bEnabled;
#endif	
}

bool UGMCE_OrganicMovementCmp::IsTrajectoryDebugEnabled() const
{
#if ENABLE_DRAW_DEBUG || WITH_EDITORONLY_DATA
	return bDrawDebugPredictions;
#else
	return false;
#endif	
}
