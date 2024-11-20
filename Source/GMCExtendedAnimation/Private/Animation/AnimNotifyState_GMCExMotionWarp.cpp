#include "Animation/AnimNotifyState_GMCExMotionWarp.h"
#include "GMCE_MotionWarpingComponent.h"
#include "GMCE_RootMotionModifier_Warp.h"
#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif

UAnimNotifyState_GMCExMotionWarp::UAnimNotifyState_GMCExMotionWarp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RootMotionModifier = ObjectInitializer.CreateDefaultSubobject<UGMCE_RootMotionModifier_Warp>(this, TEXT("SkewWarp"));
}

void UAnimNotifyState_GMCExMotionWarp::OnBecomeRelevant(UGMCE_MotionWarpingComponent* Component,
	const UAnimSequenceBase* Animation, float StartTime, float EndTime) const
{
	UGMCE_RootMotionModifier* NewModifier = AddRootMotionModifier(Component, Animation, StartTime, EndTime);

	if (NewModifier)
	{
		if (!NewModifier->OnActivateDelegate.IsBound())
		{
			NewModifier->OnActivateDelegate.BindDynamic(this, &UAnimNotifyState_GMCExMotionWarp::OnRootMotionModifierActivate);
		}

		if (!NewModifier->OnUpdateDelegate.IsBound())
		{
			NewModifier->OnUpdateDelegate.BindDynamic(this, &UAnimNotifyState_GMCExMotionWarp::OnRootMotionModifierUpdate);
		}

		if (!NewModifier->OnDeactivateDelegate.IsBound())
		{
			NewModifier->OnDeactivateDelegate.BindDynamic(this, &UAnimNotifyState_GMCExMotionWarp::OnRootMotionModifierDeactivate);
		}
	}
}

UGMCE_RootMotionModifier* UAnimNotifyState_GMCExMotionWarp::AddRootMotionModifier_Implementation(
	UGMCE_MotionWarpingComponent* Component, const UAnimSequenceBase* Animation, float StartTime, float EndTime) const
{
	if (Component && RootMotionModifier)
	{
		return Component->AddModifierFromTemplate(RootMotionModifier, Animation, StartTime, EndTime);
	}

	return nullptr;
}

void UAnimNotifyState_GMCExMotionWarp::OnRootMotionModifierActivate(UGMCE_MotionWarpingComponent* Component,
	UGMCE_RootMotionModifier* Modifier)
{
	OnWarpBegin(Component, Modifier);
}

void UAnimNotifyState_GMCExMotionWarp::OnRootMotionModifierUpdate(UGMCE_MotionWarpingComponent* Component,
	UGMCE_RootMotionModifier* Modifier)
{
	OnWarpUpdate(Component, Modifier);
}

void UAnimNotifyState_GMCExMotionWarp::OnRootMotionModifierDeactivate(UGMCE_MotionWarpingComponent* Component,
	UGMCE_RootMotionModifier* Modifier)
{
	OnWarpEnd(Component, Modifier);
}

#if WITH_EDITOR
FString UAnimNotifyState_GMCExMotionWarp::GetNotifyName_Implementation() const
{
	if (!RootMotionModifier)
	{
		return FString(TEXT("GMCEx Motion Warping"));
	}

	FString ReturnValue = RootMotionModifier->GetClass()->GetName();

	FString ModifierText = RootMotionModifier->DisplayString();

	if (!ModifierText.IsEmpty())
	{
		ReturnValue += TEXT(": ");
		ReturnValue.Append(ModifierText);
	}

	return ReturnValue;
}

void UAnimNotifyState_GMCExMotionWarp::ValidateAssociatedAssets()
{
	static const FName NAME_AssetCheck("AssetCheck");

	if(const UAnimSequenceBase* ContainingAsset = Cast<UAnimSequenceBase>(GetContainingAsset()))
	{
		if (RootMotionModifier == nullptr)
		{
			FMessageLog AssetCheckLog(NAME_AssetCheck);

			const FText MessageLooping = FText::Format(
				NSLOCTEXT("AnimNotify", "MotionWarping_InvalidRootMotionModifier", "Motion Warping window in {0} doesn't have a valid RootMotionModifier"),
				FText::AsCultureInvariant(GetNameSafe(ContainingAsset)));
			AssetCheckLog.Warning()
				->AddToken(FUObjectToken::Create(ContainingAsset))
				->AddToken(FTextToken::Create(MessageLooping));

			if (GIsEditor)
			{
				AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, true);
			}
		}
	}
	else if (const UGMCE_RootMotionModifier_Warp* RootMotionModifierWarp = Cast<UGMCE_RootMotionModifier_Warp>(RootMotionModifier))
	{
		if (RootMotionModifierWarp->WarpTargetName.IsNone())
		{
			FMessageLog AssetCheckLog(NAME_AssetCheck);

			const FText MessageLooping = FText::Format(
				NSLOCTEXT("AnimNotify", "MotionWarping_InvalidWarpTargetName", "Motion Warping window in {0} doesn't specify a valid Warp Target Name"),
				FText::AsCultureInvariant(GetNameSafe(ContainingAsset)));
			AssetCheckLog.Warning()
				->AddToken(FUObjectToken::Create(ContainingAsset))
				->AddToken(FTextToken::Create(MessageLooping));

			if (GIsEditor)
			{
				AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, true);
			}
		}
	}	
}
#endif