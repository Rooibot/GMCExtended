﻿#include "Components/GMCE_OrganicMovementCmp.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Support/GMCE_UtilityLibrary.h"


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

	if (bResetMesh)
	{
		UPrimitiveComponent* CollisionComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
		if (IsValid(CollisionComponent))
		{
			CollisionComponent->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
			SetRootCollisionHalfHeight(PreviousCollisionHalfHeight, true, false);
		}

		SkeletalMesh->SetAllBodiesSimulatePhysics(false);
		SkeletalMesh->ResetAllBodiesSimulatePhysics();
		SkeletalMesh->AttachToComponent(GetPawnOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		SkeletalMesh->SetRelativeLocationAndRotation(PreviousRelativeMeshLocation, PreviousRelativeMeshRotation, false, nullptr, ETeleportType::ResetPhysics);
		bResetMesh = false;
	}
	else if (bFirstRagdollTick && GetMovementMode() == GetRagdollMode())
	{
		bFirstRagdollTick = false;

#if ENABLE_DRAW_DEBUG || WITH_EDITORONLY_DATA
		if (bDrawDebugPredictions)
		{
			const FVector InitialActor = GetLinearVelocity_GMC();
			const FVector InitialPhysics = SkeletalMesh->GetBoneLinearVelocity(FName(TEXT("root")));
			DrawDebugLine(GetWorld(), GetActorLocation_GMC(), GetActorLocation_GMC() + RagdollLinearVelocity, FColor::Red, false, 1.f, 0, 2.f);
		}
#endif
		
		UPrimitiveComponent* CollisionComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
		if (IsValid(CollisionComponent))
		{
			CollisionComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
		}
		
		PreviousCollisionHalfHeight = GetRootCollisionHalfHeight(true);
		SetRootCollisionHalfHeight(0.1f, false, false);

		SkeletalMesh->SetAllBodiesBelowSimulatePhysics(RagdollBoneName, true, true);
		SkeletalMesh->SetAllBodiesBelowLinearVelocity(RagdollBoneName, RagdollLinearVelocity, true);
		
	}
	
	if (bHadInput && !IsInputPresent())
	{
		InputStoppedAt = GetTime();
	}
	bHadInput = IsInputPresent();

	if (GetMovementMode() == EGMC_MovementMode::Grounded)
	{
		if (bPrecalculateDistanceMatches)
		{
			UpdateStopPrediction(DeltaTime);
			UpdatePivotPrediction(DeltaTime);
		}

		if (bTrajectoryEnabled)
		{
			UpdateMovementSamples();
			if (bPrecalculateFutureTrajectory)
			{
				UpdateTrajectoryPrediction();
			}
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

#if ENABLE_DRAW_DEBUG || WITH_EDITORONLY_DATA
	if (IsTrajectoryDebugEnabled() && !IsNetMode(NM_DedicatedServer) && GetMovementMode() == EGMC_MovementMode::Grounded)
	{
		const FVector ActorLocation = GetActorLocation_GMC();
		if (bTrajectoryIsStopping || (bDebugHadPreviousStop && GetLinearVelocity_GMC().IsZero()))
		{
			if (bTrajectoryIsStopping) DebugPreviousStop = ActorLocation + PredictedStopPoint;
			DrawDebugSphere(GetWorld(), DebugPreviousStop, 24.f, 12, bTrajectoryIsStopping ? FColor::Blue : FColor::Black, false, bTrajectoryIsStopping ? -1 : 1.f, 0, bTrajectoryIsStopping ? 1.f: 2.f);
			bDebugHadPreviousStop = bTrajectoryIsStopping;
		}
		
		if (bTrajectoryIsPivoting || (bDebugHadPreviousPivot && !DoInputAndVelocityDiffer()))
		{
			if (bTrajectoryIsPivoting) DebugPreviousPivot = ActorLocation + PredictedPivotPoint;
			DrawDebugSphere(GetWorld(), DebugPreviousPivot, 24.f, 12, bTrajectoryIsPivoting ? FColor::Yellow : FColor::White, false, bTrajectoryIsPivoting ? -1 : 2.f, 0, bTrajectoryIsPivoting ? 1.f : 2.f);
			bDebugHadPreviousPivot = bTrajectoryIsPivoting;
		}

		if (DoInputAndVelocityDiffer())
		{
			const FVector LinearVelocityDirection = GetLinearVelocity_GMC().GetSafeNormal();
			const FVector AccelerationDirection = UKismetMathLibrary::RotateAngleAxis(LinearVelocityDirection, InputVelocityOffsetAngle(), FVector(0.f, 0.f, 1.f));
			DrawDebugLine(GetWorld(), ActorLocation, ActorLocation + (AccelerationDirection * 120.f), FColor::Yellow, false, -1, 0, 2.f);
		}
		
		if (bTrajectoryEnabled)
		{
			PredictedTrajectory.DrawDebug(GetWorld(), GetPawnOwner()->GetActorTransform());
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
		EGMC_PredictionMode::ClientAuth_Input,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
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
		EGMC_PredictionMode::ServerAuth_Input_ServerValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::PeriodicAndOnChange_Output,
		EGMC_InterpolationFunction::Linear
	);

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

void UGMCE_OrganicMovementCmp::MovementUpdate_Implementation(float DeltaSeconds)
{
	Super::MovementUpdate_Implementation(DeltaSeconds);

	if (!IsSimulatedProxy() || IsNetMode(NM_Standalone))
	{
		// If we're a locally-controlled pawn (or the server), we have data on current input.
		bInputPresent = !GetProcessedInputVector().IsZero();
		InputVelocityOffset = UGMCE_UtilityLibrary::GetAngleDifferenceXY(GetLinearVelocity_GMC(), GetProcessedInputVector());
		CalculatedEffectiveAcceleration = GetTransientAcceleration();
	}

	if (IsSimulatedProxy())
	{
		// If we're a simulated proxy, synthesize an acceleration value from past moves.
		UpdateCalculatedEffectiveAcceleration();
	}	
}

void UGMCE_OrganicMovementCmp::GenSimulationTick_Implementation(float DeltaTime)
{
	Super::GenSimulationTick_Implementation(DeltaTime);

	// If we're being simulated, we should always synthesize an acceleration from past movements.
	UpdateCalculatedEffectiveAcceleration();
	UpdateAnimationHelperValues(DeltaTime);
}

void UGMCE_OrganicMovementCmp::GenAncillaryTick_Implementation(float DeltaTime, bool bLocalMove,
	bool bCombinedClientMove)
{
	Super::GenAncillaryTick_Implementation(DeltaTime, bLocalMove, bCombinedClientMove);
	UpdateAnimationHelperValues(DeltaTime);
}

bool UGMCE_OrganicMovementCmp::UpdateMovementModeDynamic_Implementation(FGMC_FloorParams& Floor, float DeltaSeconds)
{
	if (bWantsRagdoll || (GetMovementMode() == GetRagdollMode()))
	{
		// We may need to either enable or disable ragdoll mode.
		if (bWantsRagdoll && GetMovementMode() == EGMC_MovementMode::Grounded)
		{
			RagdollLinearVelocity = GetRagdollInitialVelocity();
			HaltMovement();
		}
		SetMovementMode(bWantsRagdoll ? GetRagdollMode() : EGMC_MovementMode::Grounded);
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
		SetMovementMode(EGMC_MovementMode::Airborne);
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
	
	Super::OnMovementModeChangedSimulated_Implementation(PreviousMovementMode);
}

void UGMCE_OrganicMovementCmp::PhysicsCustom_Implementation(float DeltaSeconds)
{
	if (GetMovementMode() == GetRagdollMode() && bShouldReplicateRagdoll)
	{
		const FVector BoneLocation = SkeletalMesh->GetBoneLocation(RagdollBoneName);
		const FVector BoneVelocity = SkeletalMesh->GetBoneLinearVelocity(RagdollBoneName) * FVector(1.f, 1.f, 0.f);
		
		
		if (GetOwnerRole() == ROLE_Authority)
		{
			// As the server, we need to be the authority.
			
			// Set our goal for clients to use.
			CurrentRagdollGoal = BoneLocation;
		}
		else if (!CurrentRagdollGoal.IsZero() && !BoneVelocity.IsNearlyZero())
		{
			// We're a client, so figure out what needs to be done to shift the pelvis to match.
			const FVector Delta = CurrentRagdollGoal - BoneLocation;

			if (Delta.Size() > 2.f)
			{
				const FVector PelvisToComponent = SkeletalMesh->GetComponentLocation() - BoneLocation;
				SkeletalMesh->MoveComponent(Delta + PelvisToComponent, SkeletalMesh->GetComponentQuat(), false, nullptr, MOVECOMP_NoFlags, ETeleportType::None);
			}
		}

		if (!IsSimulatedProxy())
		{
			if (!BoneVelocity.IsNearlyZero())
			{
				// Find what the 'ground' is here. We do this on the affected client as well to ensure
				// a smooth camera.
				FVector NewLocation = BoneLocation;
				NewLocation.Z = UpdatedComponent->GetComponentLocation().Z;

				if (const FHitResult SweepResult =
					SweepRootCollisionSingleByChannel(
						FVector::DownVector,
						FMath::Clamp(BasedMovement.GetMaxHeight(), MIN_ACTOR_BASE_TRACE_LENGTH, UE_BIG_NUMBER),
						FVector::ZeroVector,
						FQuat::Identity,
						UpdatedComponent->GetCollisionObjectType()
					); SweepResult.bBlockingHit)
				{
					NewLocation.Z = SweepResult.Location.Z;
				}

				if (NewLocation.Z - BoneLocation.Z > PreviousCollisionHalfHeight)
				{
					NewLocation.Z = BoneLocation.Z + PreviousCollisionHalfHeight;
				}
				
				// Move our character to stay with the pelvis. We do this on the client, too, to make the
				// overall effect smooth.
				const FVector Delta = NewLocation - UpdatedComponent->GetComponentLocation();

				if (Delta.Size() > 0.5f)
				{
					FHitResult GroundResult;
					SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), false, GroundResult);
				}
			}
			else if (GetOwnerRole() == ROLE_Authority)
			{
				FVector NewLocation = UpdatedComponent->GetComponentLocation();
				NewLocation.Z = BoneLocation.Z;
				
				FHitResult GroundHit;
				const FVector LineTraceStart = NewLocation;
				const FVector LineTraceEnd = LineTraceStart + FVector::DownVector * 50.f;
				FCollisionQueryParams CollisionQueryParams(NAME_None, false, GetOwner());
				CollisionQueryParams.AddIgnoredActors(UpdatedPrimitive->GetMoveIgnoreActors());
				CollisionQueryParams.AddIgnoredComponents(UpdatedPrimitive->GetMoveIgnoreComponents());
				const auto& CollisionResponseParams = UpdatedComponent->GetCollisionResponseToChannels();
				if (const auto& World = GetWorld())
				{
					World->LineTraceSingleByChannel(
					  GroundHit,
					  LineTraceStart,
					  LineTraceEnd,
					  UpdatedComponent->GetCollisionObjectType(),
					  CollisionQueryParams,
					  CollisionResponseParams
					);
				}

				if (GroundHit.bBlockingHit)
				{
					NewLocation.Z = GroundHit.Location.Z + PreviousCollisionHalfHeight;
					SafeMoveUpdatedComponent(NewLocation - UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat(), false, GroundHit);
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

			if (Solver->PerformMovement(State, DeltaSeconds))
			{
				// Solver wants to maintain control. However, make sure to check if our active solver tag needs to
				// change. This may be true for something like the climbing solver, which handles both climbing
				// and mantling.
				if (const FGameplayTag& NewSolverTag = Solver->GetPreferredSolverTag(); NewSolverTag != CurrentActiveSolverTag)
				{
					// Solver is changing into some different sub-mode.
					OnSolverChangedMode(NewSolverTag, CurrentActiveSolverTag);
					CurrentActiveSolverTag = NewSolverTag;
				}
			}
			else
			{
				OnSolverChangedMode(FGameplayTag::EmptyTag, CurrentActiveSolverTag);
				CurrentActiveSolverTag = FGameplayTag::EmptyTag;
			}
		}
		else
		{
			// No valid solver available. Bail.
			SetMovementMode(EGMC_MovementMode::Airborne);
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
	// If we're using "Require Facing Before Move" and we're on the ground and we're not currently
	// moving, we want to check the direction we're TRYING to face and see if we're offset at all.
	if (bRequireFacingBeforeMove && IsMovingOnGround() && Velocity.IsNearlyZero())
	{
		if (bOrientToControlRotationDirection)
		{
			FVector ControlDirection = GetControllerRotation_GMC().Vector();
			const float ControlAngle = FMath::Abs(UGMCE_UtilityLibrary::GetAngleDifferenceXY(ControlDirection, UpdatedComponent->GetForwardVector()));
			if (ControlAngle > TurnInPlaceAngleThreshold)
			{
				Velocity = FVector::ZeroVector;
				HandleTurnInPlace(DeltaSeconds);
				return;
			}
		}
		else
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

	Super::CalculateVelocity(DeltaSeconds);

	if (!Velocity.IsNearlyZero())
	{
		TurnInPlaceSecondsAccumulated = 0.f;
		TurnInPlaceDelayedDirection = FVector::ZeroVector;
	}
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

	if (ShouldTurnInPlace(RootMotionMetaData))
	{
		HandleTurnInPlace(DeltaSeconds);
		return;
	}

	// Otherwise just let GMC handle it as normal.
	Super::ApplyRotation(bIsDirectBotMove, RootMotionMetaData, DeltaSeconds);
}
#pragma endregion 

#pragma region Animation Support

void UGMCE_OrganicMovementCmp::MontageUpdate(float DeltaSeconds)
{
  bHasRootMotion = false;
  
  if (!SkeletalMesh || !MontageTracker.HasActiveMontage())
  {
    MontageTracker.ClearActiveMontage();
    RootMotionParams.Clear();
    return;
  }

  if (MontageTracker.bMontagePaused)
  {
    return;
  }
  
  FGMC_AnimMontageInstance MontageInstance{MontageTracker.Montage, MontageTracker.MontagePosition, MontageTracker.MontagePlayRate, true};
  
  bool bFinishedBlendIn = false;
  bool bStartedBlendOut = false;
  
  TArray<const FAnimNotifyEvent*> MontageNotifyBeginEvents{};
  TArray<const FAnimNotifyEvent*> MontageNotifyEndEvents{};
  
  {
    gmc_ck(MontageTracker.HasActiveMontage())
    gmc_ck(!MontageTracker.bMontageEnded)
    gmc_ck(!MontageTracker.bMontagePaused)
  
    MontageTracker.bMontageEnded = MontageInstance.Advance(
      DeltaSeconds,
      RootMotionParams,
      bFinishedBlendIn,
      bStartedBlendOut,
      MontageNotifyBeginEvents,
      MontageNotifyEndEvents
    );
  
    MontageTracker.MontagePosition = MontageInstance.GetPosition();
  }
  
  gmc_ck(MontageTracker.MontagePosition >= 0.)
  gmc_ck(MontageTracker.MontagePosition <= MontageTracker.Montage->GetPlayLength())
  
  const FGMC_RootMotionExtractionSettings ExtractionSettings = GetRootMotionExtractionMetaData(MontageTracker.Montage);
  
  PreProcessRootMotion(MontageInstance, RootMotionParams, ExtractionSettings, DeltaSeconds);
  
  if (RootMotionParams.bHasRootMotion)
  {
    bHasRootMotion = true;
  
    // The root motion translation can be scaled by the user.
    RootMotionParams.ScaleRootMotionTranslation(GetAnimRootMotionTranslationScale());
  
    // Root motion calculations from motion warping
  	FTransform PreProcessedRootMotion;
  	if (ProcessRootMotionPreConvertToWorld.IsBound())
  	{
  		PreProcessedRootMotion = ProcessRootMotionPreConvertToWorld.Execute(RootMotionParams.GetRootMotionTransform(), this, DeltaSeconds);
  	}
  	else
  	{
  		PreProcessedRootMotion = RootMotionParams.GetRootMotionTransform();
  	}
    const FTransform WorldSpaceRootMotionTransform = SkeletalMesh->ConvertLocalRootMotionToWorld(PreProcessedRootMotion);
  
    // Save the root motion transform in world space.
    RootMotionParams.Set(WorldSpaceRootMotionTransform);
  
    // Calculate the root motion velocity from the world space root motion translation.
    CalculateAnimRootMotionVelocity(ExtractionSettings, DeltaSeconds);
  
    // Apply the root motion rotation now. Translation is applied with the next update from the calculated velocity. Splitting root motion translation and
    // rotation up like this may not be optimal but the alternative is to replicate the rotation for replay which is far more undesirable.
    ApplyAnimRootMotionRotation(ExtractionSettings, DeltaSeconds);
  }
  
  RootMotionParams.Clear();
  
  CallMontageEvents(MontageTracker, bFinishedBlendIn, bStartedBlendOut, MontageNotifyBeginEvents, MontageNotifyEndEvents, DeltaSeconds);
  
  gmc_ck(!MontageTracker.bStartedNewMontage)
  gmc_ck(!MontageTracker.bMontageEnded)
  gmc_ck(!MontageTracker.bInterruptedPreviousMontage)	
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
#pragma endregion


bool UGMCE_OrganicMovementCmp::IsInputPresent(bool bAllowGrace) const
{
	if (IsSimulatedProxy() && bAllowGrace)
	{
		return bInputPresent || (GetTime() - InputStoppedAt < 0.01);
	}

	return bInputPresent;
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

	PredictedPivotPoint = PredictGroundedPivotLocation(GetCurrentEffectiveAcceleration(), GetLinearVelocity_GMC(), Rotation, GroundFriction, DeltaTime, FMath::Clamp(PivotPredictionAngleThreshold, 90.f, 179.f));
	bTrajectoryIsPivoting = !PredictedPivotPoint.IsZero() && IsInputPresent() && DoInputAndVelocityDiffer();	
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
	bool bIncludeHistory) const
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
	Predictions.Samples.Add(GetMovementSampleFromCurrentState());

	FRotator RotationVelocity;
	FVector PredictedAcceleration;
	GetCurrentAccelerationRotationVelocityFromHistory(PredictedAcceleration, RotationVelocity);
	FRotator RotationVelocityPerSample = RotationVelocity * TimePerSample;

	const float BrakingDeceleration = GetBrakingDeceleration();
	const float BrakingFriction = GroundFriction;
	const float MaxSpeed = GetMaxSpeed();

	const bool bZeroFriction = BrakingFriction == 0.f;
	const bool bNoBrakes = BrakingDeceleration == 0.f;
	
	FVector CurrentLocation = FromOrigin.GetLocation();
	FRotator CurrentRotation = FromOrigin.GetRotation().Rotator();
	FVector CurrentVelocity = GetLinearVelocity_GMC();

	for (int32 Idx = 0; Idx < TotalSimulatedSamples; Idx++)
	{
		if (!RotationVelocityPerSample.IsNearlyZero())
		{
			PredictedAcceleration = RotationVelocityPerSample.RotateVector(PredictedAcceleration);
			CurrentRotation += RotationVelocityPerSample;
			RotationVelocityPerSample.Yaw /= 1.1f;
		}

		if (PredictedAcceleration.IsNearlyZero())
		{
			if (!CurrentVelocity.IsNearlyZero())
			{
				const FVector Deceleration = bNoBrakes ? FVector::ZeroVector : -BrakingDeceleration * CurrentVelocity.GetSafeNormal();

				constexpr float MaxPredictedTrajectoryTimeStep = 1.f / 33.f;
				const FVector PreviousVelocity = CurrentVelocity;

				float RemainingTime = TimePerSample;
				while (RemainingTime >= 1e-6f && !CurrentVelocity.IsZero())
				{
					const float dt = RemainingTime > MaxTimeStep && !bZeroFriction ?
						FMath::Min(MaxPredictedTrajectoryTimeStep, RemainingTime * 0.5f) : RemainingTime;
					RemainingTime -= dt;

					CurrentVelocity = CurrentVelocity + (-BrakingFriction * CurrentVelocity + Deceleration) * dt;
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
			CurrentVelocity = CurrentVelocity.GetClampedToMaxSize(MaxSpeed);
		}

		CurrentLocation += CurrentVelocity * TimePerSample;
		
		const FTransform NewTransform = FTransform(CurrentRotation.Quaternion(), CurrentLocation);
		const FTransform NewRelativeTransform = NewTransform.GetRelativeTransform(FromOrigin);

		SimulatedSample = FGMCE_MovementSample();
		SimulatedSample.RelativeTransform = NewRelativeTransform;
		SimulatedSample.RelativeLinearVelocity = FromOrigin.InverseTransformVectorNoScale(CurrentVelocity);
		SimulatedSample.WorldTransform = NewTransform;
		SimulatedSample.WorldLinearVelocity = CurrentVelocity;
		SimulatedSample.AccumulatedSeconds = TimePerSample * (Idx + 1);

		Predictions.Samples.Emplace(SimulatedSample);
	}

	return Predictions;	
}

void UGMCE_OrganicMovementCmp::UpdateTrajectoryPrediction()
{
	PredictedTrajectory = PredictMovementFuture(GetPawnOwner()->GetActorTransform(), true);	
}

FGMCE_MovementSample UGMCE_OrganicMovementCmp::GetMovementSampleFromCurrentState() const
{
	FTransform CurrentTransform = GetPawnOwner()->GetActorTransform();
	const FVector CurrentLocation = GetLowerBound();
	CurrentTransform.SetLocation(CurrentLocation);

	FGMCE_MovementSample Result = FGMCE_MovementSample(CurrentTransform, GetLinearVelocity_GMC());
	Result.ActorWorldRotation = GetPawnOwner()->GetActorRotation();
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
	if (GameSeconds - LastTrajectoryGameSeconds > SMALL_NUMBER)
	{
		AddNewMovementSample(GetMovementSampleFromCurrentState());
	}		
}

void UGMCE_OrganicMovementCmp::GetCurrentAccelerationRotationVelocityFromHistory(FVector& OutAcceleration,
	FRotator& OutRotationVelocity) const
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

		if (LastMovementSample.AccumulatedSeconds - Sample.AccumulatedSeconds >= 0.01f)
		{
			OutRotationVelocity = LastMovementSample.GetRotationVelocityFrom(Sample);
			OutAcceleration = LastMovementSample.GetAccelerationFrom(Sample);
			return;
		}
	}		
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

	bEnablePhysicsInteraction = !bActive;
	bFirstRagdollTick = bActive;
	bResetMesh = !bActive;
}

void UGMCE_OrganicMovementCmp::RunSolvers(float DeltaTime)
{
	FSolverState State = GetSolverState();
	State.AvailableSolvers.Reset();
	
	for (const auto& Solver : Solvers)
	{
		if (Solver->RunSolver(State, DeltaTime))
		{
			// ???
		}
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
		CurrentActiveSolver = nullptr;
		return true;
	}
	
	if (AvailableSolvers.HasTag(SolverTag))
	{
		for (const auto& Solver : Solvers)
		{
			if (SolverTag.MatchesTag(Solver->GetTag()))
			{
				FGameplayTag OldMode = CurrentActiveSolverTag;
				CurrentActiveSolverTag = Solver->GetPreferredSolverTag();
				OnSolverChangedMode(CurrentActiveSolverTag, OldMode);
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


void UGMCE_OrganicMovementCmp::SetStrafingMovement(bool bStrafingEnabled)
{
	bOrientToVelocityDirection = !bStrafingEnabled;
	bOrientToInputDirection = !bStrafingEnabled;
	bOrientToControlRotationDirection = bStrafingEnabled;
}

bool UGMCE_OrganicMovementCmp::ShouldTurnInPlace(const FGMC_RootMotionVelocitySettings& RootMotionMetaData)
{
	if(bOrientToControlRotationDirection && Velocity.IsNearlyZero() && (!HasRootMotion() || RootMotionMetaData.bApplyRotationWithRootMotion))
	{
		//auto turn in place when standing still
		if(TurnInPlaceDelay > 0.f)
			return true;
		
		//auto turn in place if going over the max angle
		if(TurnInPlaceAngleThreshold > 0)
			return true;
	}
	return false;
}

bool UGMCE_OrganicMovementCmp::ShouldContinueTurnInPlace(const float angle)
{
	return TurnInPlaceSecondsAccumulated > 0
		|| TurnInPlaceDelay > 0
		|| (TurnInPlaceAngleThreshold > 0 && angle >= TurnInPlaceAngleThreshold);
}

void UGMCE_OrganicMovementCmp::HandleTurnInPlace(float DeltaSeconds)
{
	//while not turning we compare to the control direction
	FVector controlDirection = GetControllerRotation_GMC().Vector();
	float angle = FMath::Abs(UGMCE_UtilityLibrary::GetAngleDifferenceXY(controlDirection, UpdatedComponent->GetForwardVector()));
	if (ShouldContinueTurnInPlace(angle))
	{
		TurnInPlaceSecondsAccumulated += DeltaSeconds;
			
		if (TurnInPlaceSecondsAccumulated >= TurnInPlaceDelay || IsInputPresent())
		{
			FVector targetRotation = GetControllerRotation_GMC().Vector() * FVector(1.f, 1.f, 0.f);
			targetRotation.Normalize();
			
			if (bUseSafeRotations)
			{
				RotateYawTowardsDirectionSafe(targetRotation, RotationRate, DeltaSeconds);				
			}
			else
			{
				RotateYawTowardsDirection(targetRotation, RotationRate, DeltaSeconds);
			}

			if (angle < 2.f)
			{
				TurnInPlaceSecondsAccumulated = 0.f;
			}
		}
	}
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
