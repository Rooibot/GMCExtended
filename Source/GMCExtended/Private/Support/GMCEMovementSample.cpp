#include "Support/GMCEMovementSample.h"

#include "Kismet/KismetMathLibrary.h"

void FGMCE_MovementSample::DrawDebug(const UWorld* World, const FTransform& FromOrigin, const FColor& Color) const
{
	const FTransform TransformWS = RelativeTransform * FromOrigin;
	const FVector PositionWS = TransformWS.GetTranslation();
	const FVector WorldVelocity = FromOrigin.TransformVector(RelativeLinearVelocity) * 0.025f + PositionWS;

	const float ArrowSize = UKismetMathLibrary::MapRangeClamped(RelativeLinearVelocity.Length(), 0, 800, 5, 30);
	const float ArrowThickness = UKismetMathLibrary::MapRangeClamped(RelativeLinearVelocity.Length(), 0, 800, 1, 3);
	
	DrawDebugDirectionalArrow(World, PositionWS, WorldVelocity, ArrowSize, Color, false, -1, 0, ArrowThickness);
	
	const FVector WorldFacingEnd = PositionWS + (ActorWorldTransform.GetRotation().GetForwardVector() * 10.f);
	DrawDebugLine(World, PositionWS, WorldFacingEnd, FColor::White, false, -1, 0, 1.f);
}

void FGMCE_MovementSampleCollection::DrawDebug(const UWorld* World, const FTransform& FromOrigin, const FColor& PastColor,
                                            const FColor& FutureColor) const
{
	FLinearColor PastColorLinear = PastColor.ReinterpretAsLinear();
	FLinearColor FutureColorLinear = FutureColor.ReinterpretAsLinear();

	const int32 TotalCount = Samples.Num();
	
	for (int32 Idx = 0; Idx < TotalCount; Idx++)
	{
		const float LerpPoint = static_cast<float>(Idx) / static_cast<float>(TotalCount);
		const FLinearColor TimelineColor = FLinearColor::LerpUsingHSV(PastColor, FutureColor, LerpPoint);
		
		Samples[Idx].DrawDebug(World, FromOrigin, TimelineColor.ToFColor(true));	
	}
}
