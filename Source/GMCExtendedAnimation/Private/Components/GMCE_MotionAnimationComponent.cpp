// Fill out your copyright notice in the Description page of Project Settings.


#include "GMCE_MotionAnimationComponent.h"

#include "GMCExtendedAnimationLog.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_RootMotionPathHolder.h"
#include "AnimNodes/AnimNode_RandomPlayer.h"


// Sets default values for this component's properties
UGMCE_MotionAnimationComponent::UGMCE_MotionAnimationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGMCE_MotionAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	OrganicMovementCmp = Cast<UGMCE_OrganicMovementCmp>(GetOwner()->GetComponentByClass(UGMCE_OrganicMovementCmp::StaticClass()));
	MotionWarpingComponent = Cast<UGMCE_MotionWarpingComponent>(GetOwner()->GetComponentByClass(UGMCE_MotionWarpingComponent::StaticClass()));
}


void UGMCE_MotionAnimationComponent::PlayMontageInternal(UAnimMontage* Montage, float StartPosition, float PlayRate)
{
	if (!OrganicMovementCmp) return;

	OrganicMovementCmp->PlayMontage_Blocking(OrganicMovementCmp->GetSkeletalMeshReference(), OrganicMovementCmp->MontageTracker, Montage, StartPosition, PlayRate);
}

// Called every frame
void UGMCE_MotionAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGMCE_MotionAnimationComponent::SetEnabled(bool bEnabled)
{
	bEnableHandling = bEnabled;

	if (bEnabled)
	{
		if (!OrganicMovementCmp)
		{
			OrganicMovementCmp = Cast<UGMCE_OrganicMovementCmp>(GetOwner()->GetComponentByClass(UGMCE_OrganicMovementCmp::StaticClass()));
		}

		if (!MotionWarpingComponent)
		{
			MotionWarpingComponent = Cast<UGMCE_MotionWarpingComponent>(GetOwner()->GetComponentByClass(UGMCE_MotionWarpingComponent::StaticClass()));
		}
	}
}

void UGMCE_MotionAnimationComponent::PredictionTick_Implementation(float DeltaSeconds)
{
}

void UGMCE_MotionAnimationComponent::PredictionTickHandler(float DeltaSeconds)
{
	if (!bEnableHandling) return;
	
	if (!bUseBlueprintEvents)
	{
		PredictionTick_Implementation(DeltaSeconds);
		return;
	}

	PredictionTick(DeltaSeconds);
}

void UGMCE_MotionAnimationComponent::AncillaryTick_Implementation(float DeltaSeconds)
{

}

void UGMCE_MotionAnimationComponent::AncillaryTickHandler(float DeltaSeconds)
{
	if (!bEnableHandling) return;
	
	if (!bUseBlueprintEvents)
	{
		AncillaryTick_Implementation(DeltaSeconds);
	}

	AncillaryTick(DeltaSeconds);	
}

void UGMCE_MotionAnimationComponent::SimulationTick_Implementation(float DeltaSeconds)
{
	if (!OrganicMovementCmp || !MotionWarpingComponent) return;
	if (!OrganicMovementCmp->HasActiveMontage(OrganicMovementCmp->MontageTracker)) return;

	SyncToCurrentMontagePosition(DeltaSeconds);	
}

void UGMCE_MotionAnimationComponent::SimulationTickHandler(float DeltaSeconds)
{
	if (!bEnableHandling) return;
	
	if (!bUseBlueprintEvents)
	{
		SimulationTick_Implementation(DeltaSeconds);
	}

	SimulationTick(DeltaSeconds);	
}

void UGMCE_MotionAnimationComponent::PostMovement_Implementation(float DeltaSeconds)
{
	if (!OrganicMovementCmp || !MotionWarpingComponent) return;

	// If we have no path and we aren't doing live (standalone) motion warping, bounce.
	if (MotionWarpingComponent->GetPathHolder()->IsEmpty() && GetOwner()->GetNetMode() != NM_Standalone) return;
	
	FTransform DeltaTransform;
	float DeltaTime;
	MotionWarpingComponent->GetLastRootMotionStep(DeltaTransform, DeltaTime, true);

	if (DeltaTransform.Equals(FTransform::Identity))
	{
		return;
	}

	FRotator CurrentRotation = OrganicMovementCmp->GetActorRotation_GMC();

	OrganicMovementCmp->MoveUpdatedComponent(DeltaTransform.GetTranslation(), CurrentRotation, false);

	if (GetOwner()->GetNetMode() == NM_Standalone)
	{
		// Normal motion warping has us covered, no need to correct.
		return;
	}
	
	// Get canonical linear velocity
	const FGMCE_MovementSample Sample = MotionWarpingComponent->GetPathHolder()->GetSampleForTime(OrganicMovementCmp->MontageTracker.MontagePosition);
	OrganicMovementCmp->SetLinearVelocity_GMC(Sample.WorldLinearVelocity);

	if (OrganicMovementCmp->IsRemotelyControlledListenServerPawn())
	{
		// Sync our smoothed, simulated proxy. The times when you want to swap from the canonical state to the
		// smoothed one during the prediction cycle are not _many_, but this is one. Here, we take the current
		// montage frame being displayed for our sync, to keep everything nice and smooth.
		OrganicMovementCmp->SV_SwapServerState();
		SyncToCurrentMontagePosition(DeltaSeconds);
		OrganicMovementCmp->SV_SwapServerState();
	}
}

void UGMCE_MotionAnimationComponent::PostMovementHandler(float DeltaSeconds)
{
	if (!bEnableHandling) return;
	
	if (!bUseBlueprintEvents)
	{
		PostMovement_Implementation(DeltaSeconds);
		return;
	}

	PostMovement(DeltaSeconds);
}

void UGMCE_MotionAnimationComponent::PostMovementSimulated_Implementation(float DeltaSeconds)
{
	if (!OrganicMovementCmp || !MotionWarpingComponent) return;
	
	if (!OrganicMovementCmp->HasActiveMontage(OrganicMovementCmp->MontageTracker)) return;

	SyncToCurrentMontagePosition(DeltaSeconds);
}

void UGMCE_MotionAnimationComponent::PostMovementSimulatedHandler(float DeltaSeconds)
{
	if (!bEnableHandling) return;
	
	if (!bUseBlueprintEvents)
	{
		PostMovementSimulated_Implementation(DeltaSeconds);
		return;
	}

	PostMovementSimulated(DeltaSeconds);
}

void UGMCE_MotionAnimationComponent::Reset()
{
	MotionWarpingComponent->GetPathHolder()->Reset();

	TargetMontage = nullptr;
	TargetStartPosition = 0.f;
	TargetPlayRate = 1.f;
	TargetOriginTransform = FTransform::Identity;
	TargetMeshRelativeTransform = FTransform::Identity;
	TargetWarpTargets = TArray<FGMCE_MotionWarpTarget>();
}

void UGMCE_MotionAnimationComponent::EnableAndPlayMontageFromOriginWithWarpTargets(UAnimMontage* Montage,
	float StartPosition, float PlayRate, FTransform OriginTransform, FTransform MeshRelativeTransform,
	TArray<FGMCE_MotionWarpTarget> WarpTargets)
{
	// We take the locally controlled location for purposes of best animation matching, but we sanity-check it
	// on the server side.
	if (!OrganicMovementCmp->GetPawnOwner()->IsLocallyControlled()) return;
	
	if (!PrecalculatePathFromOriginWithWarpTargets(Montage, StartPosition, PlayRate, OriginTransform, MeshRelativeTransform, WarpTargets))
	{
		Reset();
		return;
	}

	if (GetOwner()->GetNetMode() != NM_Standalone && !OrganicMovementCmp->IsLocallyControlledServerPawn())
	{
		SV_EnableAndPlayMontageFromOriginWithWarpTargets(Montage, StartPosition, PlayRate, OriginTransform, MeshRelativeTransform, WarpTargets);
	}
	
	TargetMontage = Montage;
	TargetStartPosition = StartPosition;
	TargetPlayRate = PlayRate;
	TargetOriginTransform = OriginTransform;
	TargetMeshRelativeTransform = MeshRelativeTransform;
	TargetWarpTargets = WarpTargets;

	SetEnabled(true);
	PlayMontageInternal(Montage, StartPosition, PlayRate);
}

void UGMCE_MotionAnimationComponent::SV_EnableAndPlayMontageFromOriginWithWarpTargets_Implementation(
	UAnimMontage* Montage, float StartPosition, float PlayRate, FTransform OriginTransform,
	FTransform MeshRelativeTransform, const TArray<FGMCE_MotionWarpTarget>& WarpTargets)
{
	if (!PrecalculatePathFromOriginWithWarpTargets(Montage, StartPosition, PlayRate, OriginTransform, MeshRelativeTransform, WarpTargets))
	{
		Reset();
		return;
	}

	TargetMontage = Montage;
	TargetStartPosition = StartPosition;
	TargetPlayRate = PlayRate;
	TargetOriginTransform = OriginTransform;
	TargetMeshRelativeTransform = MeshRelativeTransform;
	TargetWarpTargets = WarpTargets;

	SetEnabled(true);
	OrganicMovementCmp->SV_SwapServerState();
	PlayMontageInternal(Montage, StartPosition, PlayRate);	
	OrganicMovementCmp->SV_SwapServerState();
}


bool UGMCE_MotionAnimationComponent::PrecalculatePathFromOriginWithWarpTargets(UAnimMontage* Montage,
                                                                               float StartPosition, float PlayRate, FTransform OriginTransform, FTransform MeshRelativeTransform,
                                                                               TArray<FGMCE_MotionWarpTarget> WarpTargets)
{
	if (!OrganicMovementCmp || !MotionWarpingComponent) return false;

	// If the offset is more than we allow in our current error tolerances, bail.
	if (GetOwner()->GetNetMode() != NM_Standalone && (OriginTransform.GetLocation() - OrganicMovementCmp->GetActorLocation_GMC()).Length() > FMath::Max(OrganicMovementCmp->ReplicationSettings.DefaultErrorTolerances.ActorLocation, 15.f))
	{
		UE_LOG(LogGMCExAnimation, Warning, TEXT("[%s] At %s but got montage start point of %s, off by %f with a tolerance of %f."),
			*OrganicMovementCmp->GetComponentDescription(), *OrganicMovementCmp->GetActorLocation_GMC().ToCompactString(), *OriginTransform.GetLocation().ToCompactString(), (OriginTransform.GetLocation() - OrganicMovementCmp->GetActorLocation_GMC()).Length(),
			OrganicMovementCmp->ReplicationSettings.DefaultErrorTolerances.ActorLocation)
		return false;
	}
	
	MotionWarpingComponent->PrecalculatePathWithWarpTargets(Montage, StartPosition, PlayRate, OriginTransform, MeshRelativeTransform, WarpTargets, false);
	return true;
}

void UGMCE_MotionAnimationComponent::SyncToCurrentMontagePosition(float DeltaSeconds, bool bUseMontageTracker, float OverrideTime)
{
	// No path, nothing to do.
	if (MotionWarpingComponent->GetPathHolder()->IsEmpty()) return;

	// No movement component or skeletal mesh, nothing to do.
	if (!OrganicMovementCmp || !OrganicMovementCmp->GetSkeletalMeshReference()) return;

	float OutBlendTime = 0.f;
	bool bWantsBlendOut = MotionWarpingComponent->GetPathHolder()->GetPredictedPositionForBlendOut(OutBlendTime);

	float CurrentMontagePosition = -1.f;
	if (OverrideTime > 0.f)
	{
		CurrentMontagePosition = OverrideTime;
	}
	else
	{
		if (bUseMontageTracker)
		{
			CurrentMontagePosition = OrganicMovementCmp->MontageTracker.MontagePosition;
		}
		else
		{
			UAnimInstance* AnimInstance = OrganicMovementCmp->GetSkeletalMeshReference()->GetAnimInstance();
			if (!AnimInstance || !AnimInstance->IsAnyMontagePlaying()) { return; }

			CurrentMontagePosition = AnimInstance->Montage_GetPosition(AnimInstance->GetCurrentActiveMontage());
		}
	}

	// If we're past our blend out time, we truncate to that time.
	CurrentMontagePosition = FMath::Min(OutBlendTime, CurrentMontagePosition);

	if (CurrentMontagePosition == 0.f)
	{
		// We're at the origin point, where we started, so we don't need to move.
		return;
	}

	const FVector OldLocation = OrganicMovementCmp->GetActorLocation_GMC();

	FTransform ActorTransform;
	FTransform ComponentTransform;
	
	if (!MotionWarpingComponent->GetPathHolder()->GetTransformsAtPositionWithBlendOut(OrganicMovementCmp, CurrentMontagePosition, ComponentTransform, ActorTransform, bWantsBlendOut, OutBlendTime))
	{
		// No valid transforms! Bail.
		return;
	}

	const FGMCE_MovementSample Sample = MotionWarpingComponent->GetPathHolder()->GetSampleForTime(CurrentMontagePosition);

	FRotator CurrentRotation = OrganicMovementCmp->GetActorRotation_GMC();
	// Set our velocity and position.
	OrganicMovementCmp->SetLinearVelocity_GMC(Sample.WorldLinearVelocity);
	OrganicMovementCmp->UpdatedComponent->SetWorldLocationAndRotation(ActorTransform.GetLocation(), CurrentRotation);
	
	if (OrganicMovementCmp->IsSimulatedPawn())
	{
		// If we're being smoothed, handle that as well.
		FHitResult Hit;
		OrganicMovementCmp->SetComponentWorldLocationWithSmoothOffset(ComponentTransform.GetLocation(), false, Hit, false, OrganicMovementCmp->GetSkeletalMeshReference(), FVector(0.f, 0.f, - OrganicMovementCmp->GetRootCollisionHalfHeight(true)));
	}
}

