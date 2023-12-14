/* ROOIBOT CORE FRAMEWORK
 * Copyright 2023, Rooibot Games, LLC. - All rights reserved.
 *
 * The URGTrajectoryMovementComponent and support files have been made
 * available for the use of other licensees of GRIMTEC's Unreal Engine 5
 * plugin "General Movement Component v2". They may be redistributed to 
 * other GMCv2 licensees, provided this notice remains intact.
 *
 * Questions can be addressed to Rachel Blackman at either 
 * rachel.blackman@rooibot.com or as "Packetdancer" on Discord.
 */

#include "Support/GMCEMovementSample.h"

void FGMCE_MovementSample::DrawDebug(const UWorld* World, const FTransform& FromOrigin, const FColor& Color) const
{
	const FTransform TransformWS = RelativeTransform * FromOrigin;
	const FVector PositionWS = TransformWS.GetTranslation();
	const FVector WorldVelocity = FromOrigin.TransformVector(RelativeLinearVelocity) * 0.025f + PositionWS;

	DrawDebugDirectionalArrow(World, PositionWS, WorldVelocity, 20.f, Color, false, -1, 0, 2.f);	
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
