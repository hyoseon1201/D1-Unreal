// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ManaRegeneration.generated.h"

/**
 * 
 */
UCLASS()
class D1_API UMMC_ManaRegeneration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
public:
	UMMC_ManaRegeneration();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition IntelligenceDef;
};
