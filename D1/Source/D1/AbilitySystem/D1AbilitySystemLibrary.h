// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "D1AbilitySystemLibrary.generated.h"

class UD1OverlayWidgetController;
class UD1AttributeMenuWidgetController;

/**
 * 
 */
UCLASS()
class D1_API UD1AbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "D1AbilitySystemLibrary|WidgetController")
	static UD1OverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "D1AbilitySystemLibrary|WidgetController")
	static UD1AttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);
};
