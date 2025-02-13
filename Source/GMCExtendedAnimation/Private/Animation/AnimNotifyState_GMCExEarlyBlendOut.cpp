// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_GMCExEarlyBlendOut.h"

#include "GMCE_OrganicMovementCmp.h"

bool UAnimNotifyState_GMCExEarlyBlendOut::ShouldBlendOut(const UGMCE_OrganicMovementCmp* MovementComponent, bool bIsPredicted) const
{
	switch(Condition)
	{
	case EGMCExEarlyBlendOutCondition::Forced:
		return true;
	case EGMCExEarlyBlendOutCondition::OnInput:
		return bIsPredicted ? true : MovementComponent->IsInputPresent();
	case EGMCExEarlyBlendOutCondition::OnFalling:
		return bIsPredicted ? true : MovementComponent->GetLinearVelocity_GMC().Z < 0.f;
	}

	return false;
}

FString UAnimNotifyState_GMCExEarlyBlendOut::GetNotifyName_Implementation() const
{
	switch (Condition)
	{
	case EGMCExEarlyBlendOutCondition::Forced:
		return FString("Early Blend Out");
	case EGMCExEarlyBlendOutCondition::OnInput:
		return FString("Early Blend Out on Movement Input");
	case EGMCExEarlyBlendOutCondition::OnFalling:
		return FString("Early Blend Out when Falling");
	}
	
	return Super::GetNotifyName_Implementation();
}

void UAnimNotifyState_GMCExEarlyBlendOut::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	UGMCE_OrganicMovementCmp* MovementComponent = Cast<UGMCE_OrganicMovementCmp>(MeshComp->GetOwner()->GetComponentByClass(UGMCE_OrganicMovementCmp::StaticClass()));

	if (!MovementComponent) return;

	if (ShouldBlendOut(MovementComponent, false))
	{
		MovementComponent->StopMontage(MeshComp, MovementComponent->MontageTracker, BlendOutTime);

		if (MovementComponent->IsSmoothedListenServerPawn())
		{
			// If we're a smoothed listen server pawn, we want to make sure we stop montage on both the simulated version
			// (for smoothness) and the real server version (for, y'know, general network accuracy).
			MovementComponent->SV_SwapServerState();
			MovementComponent->StopMontage(MeshComp, MovementComponent->MontageTracker, BlendOutTime);
			MovementComponent->SV_SwapServerState();			
		}

		MeshComp->GetAnimInstance()->Montage_Stop(BlendOutTime);
	}
}
