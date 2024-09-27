#include "Components/GMCE_MotionWarpingComponent.h"
#include "Animation/AnimNotifyState_GMCExMotionWarp.h"
#include "GMCExtendedAnimation.h"
#include "GMCExtendedAnimationLog.h"
#include "GMCE_MotionWarpTarget.h"
#include "GMCE_RootMotionModifier_SkewWarp.h"
#include "GMCE_RootMotionModifier_Warp.h"
#include "GMCPawn.h"

UGMCE_MotionWarpingComponent::UGMCE_MotionWarpingComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
		
}

void UGMCE_MotionWarpingComponent::OnBindSharedVariables_Implementation(UGMCE_CoreComponent* BaseComponent)
{
	WarpTargetContainerInstance = FInstancedStruct::Make<FGMCE_MotionWarpTargetContainer>();
	
	BI_TargetUpdateBinding = BaseComponent->BindInstancedStruct(
		WarpTargetContainerInstance,
		EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
		EGMC_CombineMode::CombineIfUnchanged,
		EGMC_SimulationMode::Periodic_Output,
		EGMC_InterpolationFunction::TargetValue
		);	
}

void UGMCE_MotionWarpingComponent::AddOrUpdateWarpTarget(FGMCE_MotionWarpTarget& Target)
{
	// Actually update our local records.
	AddOrUpdateWarpTarget_Internal(Target);
}

void UGMCE_MotionWarpingComponent::AddOrUpdateWarpTargetFromTransform(FName WarpTargetName, FTransform TargetTransform)
{
	FGMCE_MotionWarpTarget Target = FGMCE_MotionWarpTarget(WarpTargetName, TargetTransform);
	AddOrUpdateWarpTarget_Internal(Target);
}

void UGMCE_MotionWarpingComponent::AddOrUpdateWarpTargetFromComponent(FName WarpTargetName, USceneComponent* Component,
	FName BoneName, bool bFollowComponent)
{
	FGMCE_MotionWarpTarget Target = FGMCE_MotionWarpTarget(WarpTargetName, Component, BoneName, bFollowComponent);
	AddOrUpdateWarpTarget_Internal(Target);	
}

void UGMCE_MotionWarpingComponent::AddOrUpdateWarpTargetFromLocation(FName WarpTargetName, FVector Location)
{
	FGMCE_MotionWarpTarget Target = FGMCE_MotionWarpTarget(WarpTargetName, FTransform(Location));
	AddOrUpdateWarpTarget_Internal(Target);
}

void UGMCE_MotionWarpingComponent::AddOrUpdateWarpTargetFromLocationAndRotation(FName WarpTargetName, FVector Location,
	FRotator Rotation)
{
	FGMCE_MotionWarpTarget Target = FGMCE_MotionWarpTarget(WarpTargetName, FTransform(Rotation, Location));
	AddOrUpdateWarpTarget_Internal(Target);
}

void UGMCE_MotionWarpingComponent::RemoveWarpTarget(FGMCE_MotionWarpTarget& Target)
{
	RemoveWarpTarget_Internal(Target.Name);
}

void UGMCE_MotionWarpingComponent::RemoveWarpTargetByName(FName Name)
{
	RemoveWarpTarget_Internal(Name);
}

void UGMCE_MotionWarpingComponent::RemoveAllWarpTargets()
{
	RemoveAllWarpTargets_Internal();
}

bool UGMCE_MotionWarpingComponent::ContainsModifier(const UAnimSequenceBase* Animation, float StartTime,
	float EndTime) const
{
	return Modifiers.ContainsByPredicate([=](const UGMCE_RootMotionModifier* Modifier)
		{
			return (Modifier->AnimationSequence == Animation && Modifier->StartTime == StartTime && Modifier->EndTime == EndTime);
		});
}

int32 UGMCE_MotionWarpingComponent::AddModifier(UGMCE_RootMotionModifier* Modifier)
{
	if (ensureAlways(Modifier))
	{
		UE_LOG(LogGMCExAnimation, Verbose, TEXT("Motion Warping: Modifier added. %s"), *Modifier->ToString())

		return Modifiers.Add(Modifier);
	}

	return INDEX_NONE;
}

void UGMCE_MotionWarpingComponent::DisableAllRootMotionModifiers()
{
	if (Modifiers.Num() > 0)
	{
		for (const auto& Modifier : Modifiers)
		{
			Modifier->SetState(EGMCE_RootMotionModifierState::Disabled);
		}
	}
}

UGMCE_RootMotionModifier* UGMCE_MotionWarpingComponent::AddModifierFromTemplate(UGMCE_RootMotionModifier* Template,
	const UAnimSequenceBase* Animation, float StartTime, float EndTime)
{
	if (ensureAlways(Template))
	{
		FObjectDuplicationParameters Params(Template, this);
		UGMCE_RootMotionModifier* NewModifier = CastChecked<UGMCE_RootMotionModifier>(StaticDuplicateObjectEx(Params));

		NewModifier->AnimationSequence = Animation;
		NewModifier->StartTime = StartTime;
		NewModifier->EndTime = EndTime;

		AddModifier(NewModifier);
		
		return NewModifier;
	}

	return nullptr;
}

void UGMCE_MotionWarpingComponent::Update(float DeltaSeconds)
{
	AGMC_Pawn* Pawn = GetOwningPawn();
	check(Pawn);

	FGMCE_MotionWarpContext Context;
	Context.DeltaSeconds = DeltaSeconds;
	UGMCE_OrganicMovementCmp *Component = GetMovementComponent();
	check(Component);

	if (FAnimMontageInstance* RootMotionMontageInstance = MotionWarpSubject->GetRootMotionAnimMontageInstance(MotionWarpSubject->MotionWarping_GetMeshComponent()))
	{
		const UAnimMontage* Montage = RootMotionMontageInstance->Montage;
		check(Montage);

		Context.Animation = Montage;
		Context.CurrentPosition = RootMotionMontageInstance->GetPosition();
		Context.PreviousPosition = RootMotionMontageInstance->GetPreviousPosition();
		Context.Weight = RootMotionMontageInstance->GetWeight();
		Context.PlayRate = RootMotionMontageInstance->GetPlayRate();
	}

	if (Context.Animation.IsValid())
	{
		const UAnimSequenceBase* Animation = Context.Animation.Get();
		const float PreviousPosition = Context.PreviousPosition;
		const float CurrentPosition = Context.CurrentPosition;

		for (const FAnimNotifyEvent& NotifyEvent : Animation->Notifies)
		{
			const UAnimNotifyState_GMCExMotionWarp* MotionWarpNotify = NotifyEvent.NotifyStateClass ? Cast<UAnimNotifyState_GMCExMotionWarp>(NotifyEvent.NotifyStateClass) : nullptr;
			if (MotionWarpNotify)
			{
				if (MotionWarpNotify->RootMotionModifier == nullptr)
				{
					UE_LOG(LogGMCExAnimation, Warning, TEXT("Motion Warping: a warping window in %s lacks a valid root motion modifier."),
						*GetNameSafe(Context.Animation.Get()))
					continue;
				}

				const float StartTime = FMath::Clamp(NotifyEvent.GetTriggerTime(), 0.f, Animation->GetPlayLength());
				const float EndTime = FMath::Clamp(NotifyEvent.GetEndTriggerTime(), 0.f, Animation->GetPlayLength());

				if (PreviousPosition >= StartTime && PreviousPosition < EndTime)
				{
					if (!ContainsModifier(Animation, StartTime, EndTime))
					{
						MotionWarpNotify->OnBecomeRelevant(this, Animation, StartTime, EndTime);
					}
				}
			}			
		}

		if (bSearchForWindowsInAnims)
		{
			if (const UAnimMontage* Montage = Cast<const UAnimMontage>(Context.Animation.Get()))
			{
				for (int32 SlotIdx = 0; SlotIdx < Montage->SlotAnimTracks.Num(); SlotIdx++)
				{
					const FAnimTrack& AnimTrack = Montage->SlotAnimTracks[SlotIdx].AnimTrack;

					if (const FAnimSegment* AnimSegment = AnimTrack.GetSegmentAtTime(PreviousPosition))
					{
						if (const UAnimSequenceBase* AnimReference = AnimSegment->GetAnimReference())
						{
							for (const FAnimNotifyEvent& NotifyEvent : AnimReference->Notifies)
							{
								const UAnimNotifyState_GMCExMotionWarp* MotionWarpNotify = NotifyEvent.NotifyStateClass ? Cast<UAnimNotifyState_GMCExMotionWarp>(NotifyEvent.NotifyStateClass) : nullptr;
								if (MotionWarpNotify)
								{
									if (MotionWarpNotify->RootMotionModifier == nullptr)
									{
										UE_LOG(LogGMCExAnimation, Warning, TEXT("Motion Warping: a motion warping window in %s lacks a valid root motion modifier."), *GetNameSafe(AnimReference))
										continue;
									}

									const float NotifyStartTime = FMath::Clamp(NotifyEvent.GetTriggerTime(), 0.f, AnimReference->GetPlayLength());
									const float NotifyEndTime = FMath::Clamp(NotifyEvent.GetEndTriggerTime(), 0.f, AnimReference->GetPlayLength());

									// Put them in montage context.
									const float StartTime = (NotifyStartTime - AnimSegment->AnimStartTime) + AnimSegment->StartPos;
									const float EndTime = (NotifyEndTime - AnimSegment->AnimStartTime) + AnimSegment->StartPos;

									if (PreviousPosition >= StartTime && PreviousPosition < EndTime)
									{
										if (!ContainsModifier(Montage, StartTime, EndTime))
										{
											MotionWarpNotify->OnBecomeRelevant(this, Montage, StartTime, EndTime);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	OnPreUpdate.Broadcast(this);

	if (Modifiers.Num() > 0)
	{
		// Run all modifier updates.
		for (UGMCE_RootMotionModifier* Modifier : Modifiers)
		{
			Modifier->Update(Context);
		}

		// Remove any modifiers now marked for removal.
		Modifiers.RemoveAll([this](const UGMCE_RootMotionModifier* Modifier)
		{
			return Modifier->GetState() == EGMCE_RootMotionModifierState::MarkedForRemoval;
		});
	}
}

void UGMCE_MotionWarpingComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwningPawn = Cast<AGMC_Pawn>(GetOwner());
	MotionWarpSubject = Cast<IGMCE_MotionWarpSubject>(OwningPawn);
	BindToMovementComponent();
}

void UGMCE_MotionWarpingComponent::BindToMovementComponent()
{
	if (MotionWarpSubject && !MovementComponent)
	{
		MovementComponent = MotionWarpSubject->GetGMCExMovementComponent();
		if (!MovementComponent)
		{
			// Things might've been added in blueprint that we couldn't previously obtain.
			MotionWarpSubject->MotionWarping_RecacheValues();
			MovementComponent = MotionWarpSubject->GetGMCExMovementComponent();
		}
		
		if (MovementComponent)
		{
			MovementComponent->ProcessRootMotionPreConvertToWorld.BindUObject(this, &UGMCE_MotionWarpingComponent::ProcessRootMotion);
		}
	}	
}

FTransform UGMCE_MotionWarpingComponent::ProcessRootMotion(const FTransform& InTransform,
                                                           UGMCE_OrganicMovementCmp* GMCMovementComponent, float DeltaSeconds)
{
	MotionWarpSubject->MotionWarping_GetMeshComponent()->GetAnimInstance();
	
	// Check for warping windows and update modifier states
	Update(DeltaSeconds);

	FTransform FinalRootMotion = InTransform;

	// Apply Local Space Modifiers
	for (UGMCE_RootMotionModifier* Modifier : Modifiers)
	{
		if (Modifier->GetState() == EGMCE_RootMotionModifierState::Active)
		{
			FinalRootMotion = Modifier->ProcessRootMotion(FinalRootMotion, DeltaSeconds);
		}
	}

	return FinalRootMotion;
}

void UGMCE_MotionWarpingComponent::OnSyncDataApplied(const FGMC_PawnState& State, EGMC_NetContext Context)
{

}

void UGMCE_MotionWarpingComponent::AddOrUpdateWarpTarget_Internal(FGMCE_MotionWarpTarget& Target)
{
	WarpTargetContainerInstance.GetMutable<FGMCE_MotionWarpTargetContainer>().AddOrUpdateTarget(Target);
}

void UGMCE_MotionWarpingComponent::RemoveWarpTarget_Internal(FName Name)
{
	WarpTargetContainerInstance.GetMutable<FGMCE_MotionWarpTargetContainer>().RemoveTargetByName(Name);
}

void UGMCE_MotionWarpingComponent::RemoveAllWarpTargets_Internal()
{
	WarpTargetContainerInstance.GetMutable<FGMCE_MotionWarpTargetContainer>().RemoveAllTargets();
}
