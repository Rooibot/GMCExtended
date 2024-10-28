#include "Components/GMCE_CoreComponent.h"

#include "GMCExtendedLog.h"
#include "GMCPawn.h"
#include "Interfaces/GMCE_SharedVariableComponent.h"

#define POST_MOVEMENT_HANDLER(TypeName, Type) \
	{ \
		TArray<FName> Names; \
		SharedVariables_##TypeName.GenerateKeyArray(Names); \
		for (auto& VariableName : Names) \
		{ \
			const auto Variable = SharedVariables_##TypeName.Find(VariableName); \
			if (Variable && Variable->WasUpdated()) \
			{ \
				OnShared##TypeName##Change.Broadcast(VariableName, Variable->CurrentValue, Variable->OldValue); \
				Variable->OldValue = Variable->CurrentValue; \
			} \
		} \
	}

// Sets default values for this component's properties
UGMCE_CoreComponent::UGMCE_CoreComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGMCE_CoreComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGMCE_CoreComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGMCE_CoreComponent::BindReplicationData_Implementation()
{
	Super::BindReplicationData_Implementation();

	// For the use case where the UpdatedComponent is being set later in the initialization,
	// UPawnMovementComponent's GetPawnOwner() will return null... which means GetGMCPawnOwner()
	// will as well. Attempt to get our owner more generically.
	const AActor *Owner = GetOwner();
	if (!Owner)
	{
		// We're not going to be able to do anything, so warn and exit.
		UE_LOG(LogGMCExtended, Warning, TEXT("Warning: %s has no valid owner."), *GetName())
		return;
	}

	// Make sure all our components get to register whatever variables they want.
	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (Component->Implements<UGMCE_SharedVariableComponent>())
		{
			IGMCE_SharedVariableComponent::Execute_OnBindSharedVariables(Component, this);
		}
	}

	if (Owner->Implements<UGMCE_SharedVariableComponent>())
	{
		IGMCE_SharedVariableComponent::Execute_OnBindSharedVariables(GetGMCPawnOwner(), this);
	}

	// Sort and bind each variable type.
	BindSharedBoolVariables();
	BindSharedHalfByteVariables();
	BindSharedByteVariables();
	BindSharedIntVariables();
	BindSharedSinglePrecisionFloatVariables();
	BindSharedCompressedSinglePrecisionFloatVariables();
	BindSharedDoublePrecisionFloatVariables();
	BindSharedCompressedDoublePrecisionFloatVariables();
	BindSharedTruncatedDoublePrecisionFloatVariables();
	BindSharedCompressedVector2DVariables();
	BindSharedCompressedVectorVariables();
	BindSharedCompressedRotatorVariables();
	BindSharedActorReferenceVariables();
	BindSharedActorComponentReferenceVariables();
	BindSharedAnimMontageReferenceVariables();
	BindSharedNameVariables();
	BindSharedGameplayTagVariables();
	BindSharedGameplayTagContainerVariables();
}

void UGMCE_CoreComponent::OnSyncDataApplied_Implementation(const FGMC_PawnState& State, EGMC_NetContext Context)
{
	Super::OnSyncDataApplied_Implementation(State, Context);

	// These states should cover all post-movement, just-updated-from-server, and simulation update scenarios.
	// This should ensure full coverage to check for variable updates (and notify any delegates) if values have
	// changed since the last time through.
	if (Context == EGMC_NetContext::LocalClientPawn_PostMoveExecution ||
		Context == EGMC_NetContext::LocalClientPawn_ServerStateAdoptedForReplay ||
		Context == EGMC_NetContext::LocalServerPawn_PostMoveExecution ||
		Context == EGMC_NetContext::RemoteServerPawn_PostMoveExecution ||
		Context == EGMC_NetContext::RemoteClientPawn_Simulation)
	{
		CheckForSharedVariableUpdates();
	}
}

void UGMCE_CoreComponent::CheckForSharedVariableUpdates()
{
	// Handle notification for any shared variable bindings where the values have changed.
	POST_MOVEMENT_HANDLER(Bool, bool)
	POST_MOVEMENT_HANDLER(HalfByte, uint8)
	POST_MOVEMENT_HANDLER(Byte, uint8)
	POST_MOVEMENT_HANDLER(Int, int32)
	POST_MOVEMENT_HANDLER(SinglePrecisionFloat, float)
	POST_MOVEMENT_HANDLER(CompressedSinglePrecisionFloat, float)
	POST_MOVEMENT_HANDLER(DoublePrecisionFloat, double)
	POST_MOVEMENT_HANDLER(CompressedDoublePrecisionFloat, double)
	POST_MOVEMENT_HANDLER(TruncatedDoublePrecisionFloat, double)
	POST_MOVEMENT_HANDLER(CompressedVector2D, FVector2D)
	POST_MOVEMENT_HANDLER(CompressedVector, FVector)
	POST_MOVEMENT_HANDLER(CompressedRotator, FRotator)
	POST_MOVEMENT_HANDLER(ActorReference, AActor*)
	POST_MOVEMENT_HANDLER(ActorComponentReference, UActorComponent*)
	POST_MOVEMENT_HANDLER(AnimMontageReference, UAnimMontage*)
	POST_MOVEMENT_HANDLER(Name, FName)
	POST_MOVEMENT_HANDLER(GameplayTag, FGameplayTag)
	POST_MOVEMENT_HANDLER(GameplayTagContainer, FGameplayTagContainer)	POST_MOVEMENT_HANDLER(Bool, bool)
	POST_MOVEMENT_HANDLER(HalfByte, uint8)
	POST_MOVEMENT_HANDLER(Byte, uint8)
	POST_MOVEMENT_HANDLER(Int, int32)
	POST_MOVEMENT_HANDLER(SinglePrecisionFloat, float)
	POST_MOVEMENT_HANDLER(CompressedSinglePrecisionFloat, float)
	POST_MOVEMENT_HANDLER(DoublePrecisionFloat, double)
	POST_MOVEMENT_HANDLER(CompressedDoublePrecisionFloat, double)
	POST_MOVEMENT_HANDLER(TruncatedDoublePrecisionFloat, double)
	POST_MOVEMENT_HANDLER(CompressedVector2D, FVector2D)
	POST_MOVEMENT_HANDLER(CompressedVector, FVector)
	POST_MOVEMENT_HANDLER(CompressedRotator, FRotator)
	POST_MOVEMENT_HANDLER(ActorReference, AActor*)
	POST_MOVEMENT_HANDLER(ActorComponentReference, UActorComponent*)
	POST_MOVEMENT_HANDLER(AnimMontageReference, UAnimMontage*)
	POST_MOVEMENT_HANDLER(Name, FName)
	POST_MOVEMENT_HANDLER(GameplayTag, FGameplayTag)
	POST_MOVEMENT_HANDLER(GameplayTagContainer, FGameplayTagContainer)			
}


// ---- shared variable implementations

// ---- Shared Variables: Bool
#pragma region

void UGMCE_CoreComponent::MakeSharedBool(const FName& VariableName, bool DefaultValue,
                                         EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                         EGMC_SimulationMode SimulationRule,
                                         EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_Bool.Contains(VariableName))
	{
		SharedVariables_Bool.Add(VariableName, TGMCE_SharedVariable<bool>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_Bool.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedBool(const FName& VariableName, bool& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedBool(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedBool(const FName& VariableName, bool& OutValue)
{
	if (!SharedVariables_Bool.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Bool.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedBool(const FName& VariableName, bool NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedBool(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedBool(const FName& VariableName, bool NewValue)
{
	if (!SharedVariables_Bool.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Bool.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedBoolVariables()
{
	TArray<FName> Names;
	SharedVariables_Bool.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_Bool.Find(VariableName))
		{
			Variable->BindIndex = BindBool(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_Bool.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: HalfByte
#pragma region
void UGMCE_CoreComponent::MakeSharedHalfByte(const FName& VariableName, uint8 DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_HalfByte.Contains(VariableName))
	{
		SharedVariables_HalfByte.Add(VariableName, TGMCE_SharedVariable<uint8>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_HalfByte.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedHalfByte(const FName& VariableName, uint8& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedHalfByte(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedHalfByte(const FName& VariableName, uint8& OutValue)
{
	if (!SharedVariables_HalfByte.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_HalfByte.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedHalfByte(const FName& VariableName, uint8 NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedHalfByte(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedHalfByte(const FName& VariableName, uint8 NewValue)
{
	if (!SharedVariables_HalfByte.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_HalfByte.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedHalfByteVariables()
{
	TArray<FName> Names;
	SharedVariables_HalfByte.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });
	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_HalfByte.Find(VariableName))
		{
			Variable->BindIndex = BindHalfByte(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_HalfByte.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: Byte
#pragma region
void UGMCE_CoreComponent::MakeSharedByte(const FName& VariableName, uint8 DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_Byte.Contains(VariableName))
	{
		SharedVariables_Byte.Add(VariableName, TGMCE_SharedVariable<uint8>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_Byte.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedByte(const FName& VariableName, uint8& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedByte(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedByte(const FName& VariableName, uint8& OutValue)
{
	if (!SharedVariables_Byte.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Byte.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedByte(const FName& VariableName, uint8 NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedByte(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedByte(const FName& VariableName, uint8 NewValue)
{
	if (!SharedVariables_Byte.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Byte.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedByteVariables()
{
	TArray<FName> Names;
	SharedVariables_Byte.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });
	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_Byte.Find(VariableName))
		{
			Variable->BindIndex = BindByte(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_Byte.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: Int
#pragma region
void UGMCE_CoreComponent::MakeSharedInt(const FName& VariableName, int32 DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_Int.Contains(VariableName))
	{
		SharedVariables_Int.Add(VariableName, TGMCE_SharedVariable<int32>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_Int.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedInt(const FName& VariableName, int32& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedInt(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedInt(const FName& VariableName, int32& OutValue)
{
	if (!SharedVariables_Int.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Int.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedInt(const FName& VariableName, int32 NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedInt(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedInt(const FName& VariableName, int32 NewValue)
{
	if (!SharedVariables_Int.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Int.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedIntVariables()
{
	TArray<FName> Names;
	SharedVariables_Int.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });
	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_Int.Find(VariableName))
		{
			Variable->BindIndex = BindInt(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_Int.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: SinglePrecisionFloat
#pragma region
void UGMCE_CoreComponent::MakeSharedSinglePrecisionFloat(const FName& VariableName, float DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_SinglePrecisionFloat.Contains(VariableName))
	{
		SharedVariables_SinglePrecisionFloat.Add(VariableName, TGMCE_SharedVariable<float>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_SinglePrecisionFloat.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedSinglePrecisionFloat(const FName& VariableName, float& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedSinglePrecisionFloat(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedSinglePrecisionFloat(const FName& VariableName, float& OutValue)
{
	if (!SharedVariables_SinglePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_SinglePrecisionFloat.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedSinglePrecisionFloat(const FName& VariableName, float NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedSinglePrecisionFloat(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedSinglePrecisionFloat(const FName& VariableName, float NewValue)
{
	if (!SharedVariables_SinglePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_SinglePrecisionFloat.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedSinglePrecisionFloatVariables()
{
	TArray<FName> Names;
	SharedVariables_SinglePrecisionFloat.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });
	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_SinglePrecisionFloat.Find(VariableName))
		{
			Variable->BindIndex = BindSinglePrecisionFloat(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_SinglePrecisionFloat.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: CompressedSinglePrecisionFloat
#pragma region
void UGMCE_CoreComponent::MakeSharedCompressedSinglePrecisionFloat(const FName& VariableName, float DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_CompressedSinglePrecisionFloat.Contains(VariableName))
	{
		SharedVariables_CompressedSinglePrecisionFloat.Add(VariableName, TGMCE_SharedVariable<float>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_CompressedSinglePrecisionFloat.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedCompressedSinglePrecisionFloat(const FName& VariableName, float& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedCompressedSinglePrecisionFloat(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedCompressedSinglePrecisionFloat(const FName& VariableName, float& OutValue)
{
	if (!SharedVariables_CompressedSinglePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedSinglePrecisionFloat.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedCompressedSinglePrecisionFloat(const FName& VariableName, float NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedCompressedSinglePrecisionFloat(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedCompressedSinglePrecisionFloat(const FName& VariableName, float NewValue)
{
	if (!SharedVariables_CompressedSinglePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedSinglePrecisionFloat.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedCompressedSinglePrecisionFloatVariables()
{
	TArray<FName> Names;
	SharedVariables_CompressedSinglePrecisionFloat.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });
	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_CompressedSinglePrecisionFloat.Find(VariableName))
		{
			Variable->BindIndex = BindCompressedSinglePrecisionFloat(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_CompressedSinglePrecisionFloat.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: DoublePrecisionFloat
#pragma region
void UGMCE_CoreComponent::MakeSharedDoublePrecisionFloat(const FName& VariableName, double DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_DoublePrecisionFloat.Contains(VariableName))
	{
		SharedVariables_DoublePrecisionFloat.Add(VariableName, TGMCE_SharedVariable<double>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_DoublePrecisionFloat.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedDoublePrecisionFloat(const FName& VariableName, double& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedDoublePrecisionFloat(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedDoublePrecisionFloat(const FName& VariableName, double& OutValue)
{
	if (!SharedVariables_DoublePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_DoublePrecisionFloat.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedDoublePrecisionFloat(const FName& VariableName, double NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedDoublePrecisionFloat(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedDoublePrecisionFloat(const FName& VariableName, double NewValue)
{
	if (!SharedVariables_DoublePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_DoublePrecisionFloat.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedDoublePrecisionFloatVariables()
{
	TArray<FName> Names;
	SharedVariables_DoublePrecisionFloat.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_DoublePrecisionFloat.Find(VariableName))
		{
			Variable->BindIndex = BindDoublePrecisionFloat(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_DoublePrecisionFloat.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: CompressedDoublePrecisionFloat
#pragma region
void UGMCE_CoreComponent::MakeSharedCompressedDoublePrecisionFloat(const FName& VariableName, double DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_CompressedDoublePrecisionFloat.Contains(VariableName))
	{
		SharedVariables_CompressedDoublePrecisionFloat.Add(VariableName, TGMCE_SharedVariable<double>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_CompressedDoublePrecisionFloat.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedCompressedDoublePrecisionFloat(const FName& VariableName, double& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedCompressedDoublePrecisionFloat(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedCompressedDoublePrecisionFloat(const FName& VariableName, double& OutValue)
{
	if (!SharedVariables_CompressedDoublePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedDoublePrecisionFloat.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedCompressedDoublePrecisionFloat(const FName& VariableName, double NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedCompressedDoublePrecisionFloat(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedCompressedDoublePrecisionFloat(const FName& VariableName, double NewValue)
{
	if (!SharedVariables_CompressedDoublePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedDoublePrecisionFloat.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedCompressedDoublePrecisionFloatVariables()
{
	TArray<FName> Names;
	SharedVariables_CompressedDoublePrecisionFloat.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_CompressedDoublePrecisionFloat.Find(VariableName))
		{
			Variable->BindIndex = BindCompressedDoublePrecisionFloat(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_CompressedDoublePrecisionFloat.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: TruncatedDoublePrecisionFloat
#pragma region
void UGMCE_CoreComponent::MakeSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_TruncatedDoublePrecisionFloat.Contains(VariableName))
	{
		SharedVariables_TruncatedDoublePrecisionFloat.Add(VariableName, TGMCE_SharedVariable<double>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_TruncatedDoublePrecisionFloat.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedTruncatedDoublePrecisionFloat(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double& OutValue)
{
	if (!SharedVariables_TruncatedDoublePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_TruncatedDoublePrecisionFloat.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedTruncatedDoublePrecisionFloat(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double NewValue)
{
	if (!SharedVariables_TruncatedDoublePrecisionFloat.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_TruncatedDoublePrecisionFloat.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedTruncatedDoublePrecisionFloatVariables()
{
	TArray<FName> Names;
	SharedVariables_TruncatedDoublePrecisionFloat.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_TruncatedDoublePrecisionFloat.Find(VariableName))
		{
			Variable->BindIndex = BindTruncatedDoublePrecisionFloat(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_TruncatedDoublePrecisionFloat.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: CompressedVector2D
#pragma region
void UGMCE_CoreComponent::MakeSharedCompressedVector2D(const FName& VariableName, FVector2D DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_CompressedVector2D.Contains(VariableName))
	{
		SharedVariables_CompressedVector2D.Add(VariableName, TGMCE_SharedVariable<FVector2D>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_CompressedVector2D.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedCompressedVector2D(const FName& VariableName, FVector2D& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedCompressedVector2D(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedCompressedVector2D(const FName& VariableName, FVector2D& OutValue)
{
	if (!SharedVariables_CompressedVector2D.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedVector2D.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedCompressedVector2D(const FName& VariableName, FVector2D NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedCompressedVector2D(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedCompressedVector2D(const FName& VariableName, FVector2D NewValue)
{
	if (!SharedVariables_CompressedVector2D.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedVector2D.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedCompressedVector2DVariables()
{
	TArray<FName> Names;
	SharedVariables_CompressedVector2D.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_CompressedVector2D.Find(VariableName))
		{
			Variable->BindIndex = BindCompressedVector2D(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_CompressedVector2D.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: CompressedVector
#pragma region
void UGMCE_CoreComponent::MakeSharedCompressedVector(const FName& VariableName, FVector DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_CompressedVector.Contains(VariableName))
	{
		SharedVariables_CompressedVector.Add(VariableName, TGMCE_SharedVariable<FVector>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_CompressedVector.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedCompressedVector(const FName& VariableName, FVector& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedCompressedVector(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedCompressedVector(const FName& VariableName, FVector& OutValue)
{
	if (!SharedVariables_CompressedVector.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedVector.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedCompressedVector(const FName& VariableName, FVector NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedCompressedVector(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedCompressedVector(const FName& VariableName, FVector NewValue)
{
	if (!SharedVariables_CompressedVector.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedVector.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedCompressedVectorVariables()
{
	TArray<FName> Names;
	SharedVariables_CompressedVector.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_CompressedVector.Find(VariableName))
		{
			Variable->BindIndex = BindCompressedVector(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_CompressedVector.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: CompressedRotator
#pragma region
void UGMCE_CoreComponent::MakeSharedCompressedRotator(const FName& VariableName, FRotator DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_CompressedRotator.Contains(VariableName))
	{
		SharedVariables_CompressedRotator.Add(VariableName, TGMCE_SharedVariable<FRotator>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_CompressedRotator.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedCompressedRotator(const FName& VariableName, FRotator& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedCompressedRotator(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedCompressedRotator(const FName& VariableName, FRotator& OutValue)
{
	if (!SharedVariables_CompressedRotator.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedRotator.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedCompressedRotator(const FName& VariableName, FRotator NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedCompressedRotator(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedCompressedRotator(const FName& VariableName, FRotator NewValue)
{
	if (!SharedVariables_CompressedRotator.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_CompressedRotator.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedCompressedRotatorVariables()
{
	TArray<FName> Names;
	SharedVariables_CompressedRotator.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_CompressedRotator.Find(VariableName))
		{
			Variable->BindIndex = BindCompressedRotator(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_CompressedRotator.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: ActorReference
#pragma region
void UGMCE_CoreComponent::MakeSharedActorReference(const FName& VariableName, AActor* DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_ActorReference.Contains(VariableName))
	{
		SharedVariables_ActorReference.Add(VariableName, TGMCE_SharedVariable<AActor*>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_ActorReference.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedActorReference(const FName& VariableName, AActor*& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedActorReference(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedActorReference(const FName& VariableName, AActor*& OutValue)
{
	if (!SharedVariables_ActorReference.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_ActorReference.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedActorReference(const FName& VariableName, AActor* NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedActorReference(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedActorReference(const FName& VariableName, AActor* NewValue)
{
	if (!SharedVariables_ActorReference.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_ActorReference.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedActorReferenceVariables()
{
	TArray<FName> Names;
	SharedVariables_ActorReference.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_ActorReference.Find(VariableName))
		{
			Variable->BindIndex = BindActorReference(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_ActorReference.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: ActorComponentReference
#pragma region
void UGMCE_CoreComponent::MakeSharedActorComponentReference(const FName& VariableName, UActorComponent* DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_ActorComponentReference.Contains(VariableName))
	{
		SharedVariables_ActorComponentReference.Add(VariableName, TGMCE_SharedVariable<UActorComponent*>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_ActorComponentReference.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedActorComponentReference(const FName& VariableName, UActorComponent*& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedActorComponentReference(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedActorComponentReference(const FName& VariableName, UActorComponent*& OutValue)
{
	if (!SharedVariables_ActorComponentReference.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_ActorComponentReference.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedActorComponentReference(const FName& VariableName, UActorComponent* NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedActorComponentReference(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedActorComponentReference(const FName& VariableName, UActorComponent* NewValue)
{
	if (!SharedVariables_ActorComponentReference.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_ActorComponentReference.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedActorComponentReferenceVariables()
{
	TArray<FName> Names;
	SharedVariables_ActorComponentReference.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_ActorComponentReference.Find(VariableName))
		{
			Variable->BindIndex = BindActorComponentReference(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_ActorComponentReference.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: AnimMontageReference
#pragma region
void UGMCE_CoreComponent::MakeSharedAnimMontageReference(const FName& VariableName, UAnimMontage* DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_AnimMontageReference.Contains(VariableName))
	{
		SharedVariables_AnimMontageReference.Add(VariableName, TGMCE_SharedVariable<UAnimMontage*>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_AnimMontageReference.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedAnimMontageReference(const FName& VariableName, UAnimMontage*& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedAnimMontageReference(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedAnimMontageReference(const FName& VariableName, UAnimMontage*& OutValue)
{
	if (!SharedVariables_AnimMontageReference.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_AnimMontageReference.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedAnimMontageReference(const FName& VariableName, UAnimMontage* NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedAnimMontageReference(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedAnimMontageReference(const FName& VariableName, UAnimMontage* NewValue)
{
	if (!SharedVariables_AnimMontageReference.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_AnimMontageReference.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedAnimMontageReferenceVariables()
{
	TArray<FName> Names;
	SharedVariables_AnimMontageReference.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_AnimMontageReference.Find(VariableName))
		{
			Variable->BindIndex = BindAnimMontageReference(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_AnimMontageReference.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: Name
#pragma region
void UGMCE_CoreComponent::MakeSharedName(const FName& VariableName, FName DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_Name.Contains(VariableName))
	{
		SharedVariables_Name.Add(VariableName, TGMCE_SharedVariable<FName>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_Name.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedName(const FName& VariableName, FName& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedName(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedName(const FName& VariableName, FName& OutValue)
{
	if (!SharedVariables_Name.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Name.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedName(const FName& VariableName, FName NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedName(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedName(const FName& VariableName, FName NewValue)
{
	if (!SharedVariables_Name.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_Name.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedNameVariables()
{
	TArray<FName> Names;
	SharedVariables_Name.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_Name.Find(VariableName))
		{
			Variable->BindIndex = BindName(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_Name.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: GameplayTag
#pragma region
void UGMCE_CoreComponent::MakeSharedGameplayTag(const FName& VariableName, FGameplayTag DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_GameplayTag.Contains(VariableName))
	{
		SharedVariables_GameplayTag.Add(VariableName, TGMCE_SharedVariable<FGameplayTag>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_GameplayTag.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedGameplayTag(const FName& VariableName, FGameplayTag& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedGameplayTag(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedGameplayTag(const FName& VariableName, FGameplayTag& OutValue)
{
	if (!SharedVariables_GameplayTag.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_GameplayTag.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedGameplayTag(const FName& VariableName, FGameplayTag NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedGameplayTag(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedGameplayTag(const FName& VariableName, FGameplayTag NewValue)
{
	if (!SharedVariables_GameplayTag.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_GameplayTag.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedGameplayTagVariables()
{
	TArray<FName> Names;
	SharedVariables_GameplayTag.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_GameplayTag.Find(VariableName))
		{
			Variable->BindIndex = BindGameplayTag(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_GameplayTag.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

// ---- Shared Variables: GameplayTagContainer
#pragma region
void UGMCE_CoreComponent::MakeSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer DefaultValue,
                                   EGMC_PredictionMode PredictionRule, EGMC_CombineMode CombineRule,
                                   EGMC_SimulationMode SimulationRule,
                                   EGMC_InterpolationFunction InterpolationRule)
{
	if (!SharedVariables_GameplayTagContainer.Contains(VariableName))
	{
		SharedVariables_GameplayTagContainer.Add(VariableName, TGMCE_SharedVariable<FGameplayTagContainer>(VariableName, DefaultValue));
		const auto Variable = SharedVariables_GameplayTagContainer.Find(VariableName);
		Variable->PredictionRule = PredictionRule;
		Variable->CombineRule = CombineRule;
		Variable->SimulationRule = SimulationRule;
		Variable->InterpolationRule = InterpolationRule;
	}
}

void UGMCE_CoreComponent::GetSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer& OutValue, bool& bOutSuccess)
{
	bOutSuccess = GetSharedGameplayTagContainer(VariableName, OutValue);
}

bool UGMCE_CoreComponent::GetSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer& OutValue)
{
	if (!SharedVariables_GameplayTagContainer.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_GameplayTagContainer.Find(VariableName))
	{
		OutValue = Variable->CurrentValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::SetSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer NewValue, bool& bOutSuccess)
{
	bOutSuccess = SetSharedGameplayTagContainer(VariableName, NewValue);
}

bool UGMCE_CoreComponent::SetSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer NewValue)
{
	if (!SharedVariables_GameplayTagContainer.Contains(VariableName)) return false;

	if (const auto Variable = SharedVariables_GameplayTagContainer.Find(VariableName))
	{
		Variable->CurrentValue = NewValue;
		return true;
	}

	return false;
}

void UGMCE_CoreComponent::BindSharedGameplayTagContainerVariables()
{
	TArray<FName> Names;
	SharedVariables_GameplayTagContainer.GenerateKeyArray(Names);
	Names.Sort([](const FName& A, const FName& B){ return A.FastLess(B); });

	for (FName& VariableName : Names)
	{
		if (const auto Variable = SharedVariables_GameplayTagContainer.Find(VariableName))
		{
			Variable->BindIndex = BindGameplayTagContainer(
				Variable->CurrentValue,
				Variable->PredictionRule,
				Variable->CombineRule,
				Variable->SimulationRule,
				Variable->InterpolationRule
			);

			if (Variable->BindIndex < 0)
			{
				SharedVariables_GameplayTagContainer.Remove(VariableName);
			}
		}
	}
}
#pragma endregion

