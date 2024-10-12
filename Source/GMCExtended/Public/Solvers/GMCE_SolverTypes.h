#pragma once
#include "CoreMinimal.h"
#include "GMCOrganicMovementComponent.h"
#include "GMCE_SolverTypes.generated.h"

class UGMCE_BaseSolver;

UENUM(BlueprintType)
enum class EGMCExtendedLogType : uint8
{
	DebugLog UMETA(DisplayName = "Debug Only Log"),
	LogVeryVerbose UMETA(DisplayName = "Very Verbose Log"),
	LogVerbose UMETA(DisplayName = "Verbose Log"),
	LogWarning UMETA(DisplayName = "Warning Log"),
	LogError UMETA(DisplayName = "Error Log")
};

USTRUCT(BlueprintType)
struct GMCEXTENDED_API FSolverState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	bool bIsPrediction { false };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	EGMC_MovementMode MovementMode { EGMC_MovementMode::Grounded };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FGameplayTag MovementTag { FGameplayTag::EmptyTag };
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector Location { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FRotator Rotation { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector LinearVelocity { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector RawInput { 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Solver State")
	FVector ProcessedInput { 0.f };

	// -- Values below here can be changed by a solver and will be acted on.
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Solver State")
	FGameplayTagContainer AvailableSolvers { };
	
};
