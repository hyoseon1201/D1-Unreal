// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "D1DamageTextComponent.generated.h"

/**
 * 
 */
UCLASS()
class D1_API UD1DamageTextComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetDamageText(float Damage, bool bCriticalHit);
};
