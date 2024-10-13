#include "Support/GMCEMovementSample.h"

#include "Kismet/KismetMathLibrary.h"

void FGMCE_MovementSample::DrawDebug(const UWorld* World, const FTransform& FromOrigin, const FColor& Color, float LifeTime) const
{
	// const FTransform TransformWS = RelativeTransform * FromOrigin;
	const FTransform TransformWS = WorldTransform;
	const FVector PositionWS = TransformWS.GetTranslation() + FVector(0.f, 0.f, 2.f);
	const FVector WorldVelocity = WorldLinearVelocity * 0.025f + PositionWS;

	const float ArrowSize = UKismetMathLibrary::MapRangeClamped(WorldLinearVelocity.Length(), 0, 800, 5, 30);
	const float ArrowThickness = UKismetMathLibrary::MapRangeClamped(WorldLinearVelocity.Length(), 0, 800, 1, 3);
	
	DrawDebugDirectionalArrow(World, PositionWS, WorldVelocity, ArrowSize, AccumulatedSeconds != 0 ? Color : FColor::White, false, (AccumulatedSeconds > 0.f) ? LifeTime : -1, 0, ArrowThickness);

	const FVector WorldFacingStart = PositionWS + (FVector::UpVector * 5.f);
	const FVector WorldFacingEnd = WorldFacingStart + (ActorWorldTransform.GetRotation().GetForwardVector() * 10.f);
	DrawDebugLine(World, WorldFacingStart, WorldFacingEnd, FColor::White, false, (AccumulatedSeconds > 0.f) ? LifeTime : -1, 0, 1.f);

	if (bUseAsMarker)
	{
		DrawDebugBox(World, PositionWS, FVector(5.f), FColor::White, false, (AccumulatedSeconds > 0.f) ? LifeTime : -1, 0, 1.f);
		DrawDebugLine(World, PositionWS + FVector::UpVector * 40, PositionWS - FVector::UpVector * 40, FColor::White, false, (AccumulatedSeconds > 0.f) ? LifeTime : -1, 0.f);
	}
}

void FGMCE_MovementSampleCollection::DrawDebug(const UWorld* World, const FTransform& FromOrigin, const FColor& PastColor, const FColor& PresentColor, const FColor& FutureColor, int PastSamples, float LifeTime) const
{
	const int32 TotalCount = Samples.Num();
	
	for (int32 Idx = 0; Idx < TotalCount; Idx++)
	{
		FLinearColor TimelineColor;
		if (PastSamples > 0)
		{
			if (Idx < PastSamples)
			{
				const float LerpPoint = static_cast<float>(Idx) / static_cast<float>(PastSamples);
				TimelineColor = FLinearColor::LerpUsingHSV(PastColor, PresentColor, LerpPoint);
			}
			else
			{
				const float LerpPoint = static_cast<float>(Idx - PastSamples) / static_cast<float>(TotalCount - PastSamples);
				TimelineColor = FLinearColor::LerpUsingHSV(PresentColor, FutureColor, LerpPoint);			
			}
		}
		else
		{
			const float LerpPoint = static_cast<float>(Idx) / static_cast<float>(TotalCount);
			TimelineColor = FLinearColor::LerpUsingHSV(PastColor, FutureColor, LerpPoint);
		}
		
		Samples[Idx].DrawDebug(World, FromOrigin, TimelineColor.ToFColor(true), LifeTime);	
	}
}
