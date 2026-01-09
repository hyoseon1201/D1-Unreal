// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_AttackPower.generated.h"

/**
 * 
 */
UCLASS()
class D1_API UMMC_AttackPower : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	UMMC_AttackPower();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:

	FGameplayEffectAttributeCaptureDefinition StrengthDef;
};
