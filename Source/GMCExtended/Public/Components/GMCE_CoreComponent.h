#pragma once

#include "CoreMinimal.h"
#include "GMCOrganicMovementComponent.h"
#include "GMCE_CoreComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedBoolChange, FName, VariableName, bool, NewValue, bool, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedHalfByteChange, FName, VariableName, uint8, NewValue, uint8, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedByteChange, FName, VariableName, uint8, NewValue, uint8, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedIntChange, FName, VariableName, int32, NewValue, int32, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedSinglePrecisionFloatChange, FName, VariableName, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedCompressedSinglePrecisionFloatChange, FName, VariableName, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedDoublePrecisionFloatChange, FName, VariableName, double, NewValue, double, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedCompressedDoublePrecisionFloatChange, FName, VariableName, double, NewValue, double, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedTruncatedDoublePrecisionFloatChange, FName, VariableName, double, NewValue, double, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedCompressedVector2DChange, FName, VariableName, FVector2D, NewValue, FVector2D, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedCompressedVectorChange, FName, VariableName, FVector, NewValue, FVector, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedCompressedRotatorChange, FName, VariableName, FRotator, NewValue, FRotator, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedActorReferenceChange, FName, VariableName, AActor*, NewValue, AActor*, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedActorComponentReferenceChange, FName, VariableName, UActorComponent*, NewValue, UActorComponent*, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedAnimMontageReferenceChange, FName, VariableName, UAnimMontage*, NewValue, UAnimMontage*, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedNameChange, FName, VariableName, FName, NewValue, FName, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedGameplayTagChange, FName, VariableName, FGameplayTag, NewValue, FGameplayTag, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSharedGameplayTagContainerChange, FName, VariableName, FGameplayTagContainer, NewValue, FGameplayTagContainer, OldValue);

class UGMCE_CoreComponent;

template<typename T>
struct TGMCE_SharedVariable
{
public:
	/// Get the name of this shared variable.
	FName GetName() const { return VariableName; }

	/// Get the current value of this shared variable.
	T GetValue() const { return CurrentValue; }

	/// Set a current value for this shared variable.
	void SetValue(T NewValue) { CurrentValue = NewValue; };

	/// Obtain the GMC bind index for this variable.
	int32 GetBindIndex() const { return BindIndex; }
	
	bool operator==(const TGMCE_SharedVariable<T>& Other ) const { return GetName() == VariableName; }
	T operator=(const T& NewValue) { CurrentValue = NewValue; return NewValue; }
	TGMCE_SharedVariable<T> operator=(const TGMCE_SharedVariable<T>& Other)
	{
		CurrentValue = Other.CurrentValue;
		return this;
	}
	
	// ReSharper disable once CppNonExplicitConversionOperator
	operator T() const { return CurrentValue; }

	TGMCE_SharedVariable<T>(const FName& Name, const T& Value)
	{
		VariableName = Name;
		CurrentValue = Value;
		BindIndex = -1;
		PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated;
		CombineRule = EGMC_CombineMode::AlwaysCombine;
		SimulationRule = EGMC_SimulationMode::Periodic_Output;
		InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour;
	}

	bool WasUpdated() const { return CurrentValue != OldValue; }
	
private:
	/// The name of this shared attribute.
	FName	VariableName;
	
	/// The current value, including any current modifiers.
	UFUNCTION()
	T		CurrentValue;

	/// The value of this variable on the previous movement loop.
	UFUNCTION()
	T		OldValue;

	int32   BindIndex;

	EGMC_PredictionMode PredictionRule;
	EGMC_CombineMode CombineRule;
	EGMC_SimulationMode SimulationRule;
	EGMC_InterpolationFunction InterpolationRule;

	friend UGMCE_CoreComponent;
	
};

#define SHARED_VARIABLES(TypeName, Type) \
	TMap<FName,TGMCE_SharedVariable<Type>> SharedVariables_##TypeName;

UCLASS(ClassGroup=(GMCExtended), meta=(BlueprintSpawnableComponent, DisplayName="GMCExtended Core Component"))
class GMCEXTENDED_API UGMCE_CoreComponent : public UGMC_OrganicMovementCmp
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGMCE_CoreComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BindReplicationData_Implementation() override;
	virtual void OnSyncDataApplied_Implementation(const FGMC_PawnState& State, EGMC_NetContext Context) override;

	virtual void CheckForSharedVariableUpdates();

	// SharedVars - Bool
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|Bool", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedBool(const FName& VariableName, bool DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
	                 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
	                 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Bool", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedBool(const FName& VariableName, UPARAM(DisplayName="Value") bool& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedBool(const FName& VariableName, bool& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Bool", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedBool(const FName& VariableName, bool NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedBool(const FName& VariableName, bool NewValue);

	void BindSharedBoolVariables();
	
#pragma endregion

	// SharedVars - HalfByte
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|HalfByte", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedHalfByte(const FName& VariableName, uint8 DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|HalfByte", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedHalfByte(const FName& VariableName, UPARAM(DisplayName="Value") uint8& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedHalfByte(const FName& VariableName, uint8& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|HalfByte", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedHalfByte(const FName& VariableName, uint8 NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedHalfByte(const FName& VariableName, uint8 NewValue);

	void BindSharedHalfByteVariables();
#pragma endregion

	// SharedVars - Byte
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|Byte", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedByte(const FName& VariableName, uint8 DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Byte", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedByte(const FName& VariableName, UPARAM(DisplayName="Value") uint8& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedByte(const FName& VariableName, uint8& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Byte", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedByte(const FName& VariableName, uint8 NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedByte(const FName& VariableName, uint8 NewValue);

	void BindSharedByteVariables();
#pragma endregion

	// SharedVars - Int
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|Int", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedInt(const FName& VariableName, int32 DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Int", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedInt(const FName& VariableName, UPARAM(DisplayName="Value") int32& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedInt(const FName& VariableName, int32& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Int", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedInt(const FName& VariableName, int32 NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedInt(const FName& VariableName, int32 NewValue);

	void BindSharedIntVariables();
#pragma endregion

	// SharedVars - SinglePrecisionFloat
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|SinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedSinglePrecisionFloat(const FName& VariableName, float DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|SinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedSinglePrecisionFloat(const FName& VariableName, UPARAM(DisplayName="Value") float& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedSinglePrecisionFloat(const FName& VariableName, float& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|SinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedSinglePrecisionFloat(const FName& VariableName, float NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedSinglePrecisionFloat(const FName& VariableName, float NewValue);

	void BindSharedSinglePrecisionFloatVariables();
#pragma endregion

	// SharedVars - CompressedSinglePrecisionFloat
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedSinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedCompressedSinglePrecisionFloat(const FName& VariableName, float DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedSinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedCompressedSinglePrecisionFloat(const FName& VariableName, UPARAM(DisplayName="Value") float& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedCompressedSinglePrecisionFloat(const FName& VariableName, float& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedSinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedCompressedSinglePrecisionFloat(const FName& VariableName, float NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedCompressedSinglePrecisionFloat(const FName& VariableName, float NewValue);

	void BindSharedCompressedSinglePrecisionFloatVariables();
#pragma endregion

	// SharedVars - DoublePrecisionFloat
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|DoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedDoublePrecisionFloat(const FName& VariableName, double DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|DoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedDoublePrecisionFloat(const FName& VariableName, UPARAM(DisplayName="Value") double& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedDoublePrecisionFloat(const FName& VariableName, double& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|DoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedDoublePrecisionFloat(const FName& VariableName, double NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedDoublePrecisionFloat(const FName& VariableName, double NewValue);

	void BindSharedDoublePrecisionFloatVariables();
#pragma endregion

	// SharedVars - CompressedDoublePrecisionFloat
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedCompressedDoublePrecisionFloat(const FName& VariableName, double DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedCompressedDoublePrecisionFloat(const FName& VariableName, UPARAM(DisplayName="Value") double& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedCompressedDoublePrecisionFloat(const FName& VariableName, double& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedCompressedDoublePrecisionFloat(const FName& VariableName, double NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedCompressedDoublePrecisionFloat(const FName& VariableName, double NewValue);

	void BindSharedCompressedDoublePrecisionFloatVariables();
#pragma endregion

	// SharedVars - TruncatedDoublePrecisionFloat
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|TruncatedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|TruncatedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, UPARAM(DisplayName="Value") double& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|TruncatedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedTruncatedDoublePrecisionFloat(const FName& VariableName, double NewValue);

	void BindSharedTruncatedDoublePrecisionFloatVariables();
#pragma endregion

	// SharedVars - CompressedVector2D
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedVector2D", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedCompressedVector2D(const FName& VariableName, FVector2D DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedVector2D", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedCompressedVector2D(const FName& VariableName, UPARAM(DisplayName="Value") FVector2D& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedCompressedVector2D(const FName& VariableName, FVector2D& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedVector2D", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedCompressedVector2D(const FName& VariableName, FVector2D NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedCompressedVector2D(const FName& VariableName, FVector2D NewValue);

	void BindSharedCompressedVector2DVariables();
#pragma endregion

	// SharedVars - CompressedVector
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedVector", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedCompressedVector(const FName& VariableName, FVector DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedVector", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedCompressedVector(const FName& VariableName, UPARAM(DisplayName="Value") FVector& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedCompressedVector(const FName& VariableName, FVector& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedVector", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedCompressedVector(const FName& VariableName, FVector NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedCompressedVector(const FName& VariableName, FVector NewValue);

	void BindSharedCompressedVectorVariables();
#pragma endregion

	// SharedVars - CompressedRotator
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedRotator", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedCompressedRotator(const FName& VariableName, FRotator DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedRotator", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedCompressedRotator(const FName& VariableName, UPARAM(DisplayName="Value") FRotator& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedCompressedRotator(const FName& VariableName, FRotator& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|CompressedRotator", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedCompressedRotator(const FName& VariableName, FRotator NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedCompressedRotator(const FName& VariableName, FRotator NewValue);

	void BindSharedCompressedRotatorVariables();
#pragma endregion

	// SharedVars - ActorReference
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|ActorReference", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedActorReference(const FName& VariableName, AActor* DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|ActorReference", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedActorReference(const FName& VariableName, UPARAM(DisplayName="Value") AActor*& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedActorReference(const FName& VariableName, AActor*& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|ActorReference", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedActorReference(const FName& VariableName, AActor* NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedActorReference(const FName& VariableName, AActor* NewValue);

	void BindSharedActorReferenceVariables();
#pragma endregion

	// SharedVars - ActorComponentReference
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|ActorComponentReference", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedActorComponentReference(const FName& VariableName, UActorComponent* DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|ActorComponentReference", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedActorComponentReference(const FName& VariableName, UPARAM(DisplayName="Value") UActorComponent*& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedActorComponentReference(const FName& VariableName, UActorComponent*& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|ActorComponentReference", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedActorComponentReference(const FName& VariableName, UActorComponent* NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedActorComponentReference(const FName& VariableName, UActorComponent* NewValue);

	void BindSharedActorComponentReferenceVariables();
#pragma endregion

	// SharedVars - AnimMontageReference
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|AnimMontageReference", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedAnimMontageReference(const FName& VariableName, UAnimMontage* DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|AnimMontageReference", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedAnimMontageReference(const FName& VariableName, UPARAM(DisplayName="Value") UAnimMontage*& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedAnimMontageReference(const FName& VariableName, UAnimMontage*& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|AnimMontageReference", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedAnimMontageReference(const FName& VariableName, UAnimMontage* NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedAnimMontageReference(const FName& VariableName, UAnimMontage* NewValue);

	void BindSharedAnimMontageReferenceVariables();
#pragma endregion

	// SharedVars - Name
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|Name", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedName(const FName& VariableName, FName DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Name", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedName(const FName& VariableName, UPARAM(DisplayName="Value") FName& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedName(const FName& VariableName, FName& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|Name", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedName(const FName& VariableName, FName NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedName(const FName& VariableName, FName NewValue);

	void BindSharedNameVariables();
#pragma endregion

	// SharedVars - GameplayTag
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|GameplayTag", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedGameplayTag(const FName& VariableName, FGameplayTag DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|GameplayTag", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedGameplayTag(const FName& VariableName, UPARAM(DisplayName="Value") FGameplayTag& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedGameplayTag(const FName& VariableName, FGameplayTag& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|GameplayTag", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedGameplayTag(const FName& VariableName, FGameplayTag NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedGameplayTag(const FName& VariableName, FGameplayTag NewValue);

	void BindSharedGameplayTagVariables();
#pragma endregion

	// SharedVars - GameplayTagContainer
#pragma region
	UFUNCTION(BlueprintCallable, Category="Shared Variables|GameplayTagContainer", meta=(AutoCreateRefTerm="VariableName"))
	void MakeSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer DefaultValue, EGMC_PredictionMode PredictionRule = EGMC_PredictionMode::ServerAuth_Output_ClientValidated,
					 EGMC_CombineMode CombineRule = EGMC_CombineMode::AlwaysCombine, EGMC_SimulationMode SimulationRule = EGMC_SimulationMode::Periodic_Output,
					 EGMC_InterpolationFunction InterpolationRule = EGMC_InterpolationFunction::NearestNeighbour);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|GameplayTagContainer", meta=(AutoCreateRefTerm="VariableName"))
	void GetSharedGameplayTagContainer(const FName& VariableName, UPARAM(DisplayName="Value") FGameplayTagContainer& OutValue,
	                UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool GetSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer& OutValue);

	UFUNCTION(BlueprintCallable, Category="Shared Variables|GameplayTagContainer", meta=(AutoCreateRefTerm="VariableName"))
	void SetSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer NewValue, UPARAM(DisplayName="Success") bool& bOutSuccess);
	bool SetSharedGameplayTagContainer(const FName& VariableName, FGameplayTagContainer NewValue);

	void BindSharedGameplayTagContainerVariables();
#pragma endregion

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|Bool", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedBoolChange OnSharedBoolChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|HalfByte", meta=(AutoCreateRefTerm="VariableName"))	
	FOnSharedHalfByteChange OnSharedHalfByteChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|Byte", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedByteChange OnSharedByteChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|Int", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedIntChange OnSharedIntChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|SinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedSinglePrecisionFloatChange OnSharedSinglePrecisionFloatChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|CompressedSinglePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedCompressedSinglePrecisionFloatChange OnSharedCompressedSinglePrecisionFloatChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|DoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedDoublePrecisionFloatChange OnSharedDoublePrecisionFloatChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|CompressedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedCompressedDoublePrecisionFloatChange OnSharedCompressedDoublePrecisionFloatChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|TruncatedDoublePrecisionFloat", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedTruncatedDoublePrecisionFloatChange OnSharedTruncatedDoublePrecisionFloatChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|CompressedVector2D", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedCompressedVector2DChange OnSharedCompressedVector2DChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|CompressedVector", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedCompressedVectorChange OnSharedCompressedVectorChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|CompressedRotator", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedCompressedRotatorChange OnSharedCompressedRotatorChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|ActorReference", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedActorReferenceChange OnSharedActorReferenceChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|ActorComponentReference", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedActorComponentReferenceChange OnSharedActorComponentReferenceChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|AnimMontageReference", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedAnimMontageReferenceChange OnSharedAnimMontageReferenceChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|Name", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedNameChange OnSharedNameChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|GameplayTag", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedGameplayTagChange OnSharedGameplayTagChange;

	UPROPERTY(BlueprintAssignable, Category="Shared Variables|GameplayTagContainer", meta=(AutoCreateRefTerm="VariableName"))
	FOnSharedGameplayTagContainerChange OnSharedGameplayTagContainerChange;

private:
	// --- External binding records for all registered-by-others bindings.
	SHARED_VARIABLES(Bool, bool)
	SHARED_VARIABLES(HalfByte, uint8)
	SHARED_VARIABLES(Byte, uint8)
	SHARED_VARIABLES(Int, int32)
	SHARED_VARIABLES(SinglePrecisionFloat, float)
	SHARED_VARIABLES(CompressedSinglePrecisionFloat, float)
	SHARED_VARIABLES(DoublePrecisionFloat, double)
	SHARED_VARIABLES(CompressedDoublePrecisionFloat, double)
	SHARED_VARIABLES(TruncatedDoublePrecisionFloat, double)
	SHARED_VARIABLES(CompressedVector2D, FVector2D)
	SHARED_VARIABLES(CompressedVector, FVector)
	SHARED_VARIABLES(CompressedRotator, FRotator)
	SHARED_VARIABLES(ActorReference, AActor*)
	SHARED_VARIABLES(ActorComponentReference, UActorComponent*)
	SHARED_VARIABLES(AnimMontageReference, UAnimMontage*)
	SHARED_VARIABLES(Name, FName)
	SHARED_VARIABLES(GameplayTag, FGameplayTag)
	SHARED_VARIABLES(GameplayTagContainer, FGameplayTagContainer)

	bool bHasFinishedBinding { true };
	
};

#undef SHARED_VARIABLES
