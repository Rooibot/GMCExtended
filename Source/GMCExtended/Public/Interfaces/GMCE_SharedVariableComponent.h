#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GMCE_SharedVariableComponent.generated.h"

// This class does not need to be modified.
UINTERFACE(meta=(DisplayName="Shared Variable Component Interface"))
class UGMCE_SharedVariableComponent : public UInterface
{
	GENERATED_BODY()
};

class UGMCE_CoreComponent;

/**
 * 
 */
class GMCEXTENDED_API IGMCE_SharedVariableComponent
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, Category="GMC Extended|Shared Variables")
	void OnBindSharedVariables(UGMCE_CoreComponent* BaseComponent);	
	
};
