// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MMC/MMC_CriticalHitDamage.h"

#include "AbilitySystem/D1AttributeSet.h"

UMMC_CriticalHitDamage::UMMC_CriticalHitDamage()
{
	LuckDef.AttributeToCapture = UD1AttributeSet::GetLuckAttribute();
	LuckDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	LuckDef.bSnapshot = false;

	DexterityDef.AttributeToCapture = UD1AttributeSet::GetDexterityAttribute();
	DexterityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	DexterityDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(LuckDef);
	RelevantAttributesToCapture.Add(DexterityDef);
}

float UMMC_CriticalHitDamage::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Luck = 0.f;
	GetCapturedAttributeMagnitude(LuckDef, Spec, EvaluationParameters, Luck);
	Luck = FMath::Max<float>(Luck, 0.f);

	float Dexterity = 0.f;
	GetCapturedAttributeMagnitude(DexterityDef, Spec, EvaluationParameters, Dexterity);
	Dexterity = FMath::Max<float>(Dexterity, 0.f);

	return (Luck * 1.5f) + (Dexterity * 0.5f) + 150.f;
}