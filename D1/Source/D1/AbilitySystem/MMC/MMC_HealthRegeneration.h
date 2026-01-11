// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_HealthRegeneration.generated.h"

/**
 * 
 */
UCLASS()
class D1_API UMMC_HealthRegeneration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
public:
	UMMC_HealthRegeneration();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition StrengthDef;
};
