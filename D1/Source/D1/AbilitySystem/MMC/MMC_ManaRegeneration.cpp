// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MMC/MMC_ManaRegeneration.h"

#include "AbilitySystem/D1AttributeSet.h"

UMMC_ManaRegeneration::UMMC_ManaRegeneration()
{
	IntelligenceDef.AttributeToCapture = UD1AttributeSet::GetIntelligenceAttribute();
	IntelligenceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	IntelligenceDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(IntelligenceDef);
}

float UMMC_ManaRegeneration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Intelligence = 0.f;
	GetCapturedAttributeMagnitude(IntelligenceDef, Spec, EvaluationParameters, Intelligence);
	Intelligence = FMath::Max<float>(Intelligence, 0.f);

	float Level = Spec.GetLevel();

	return (Intelligence * 15.f) + (Level * 5.f) + 50.f;
}