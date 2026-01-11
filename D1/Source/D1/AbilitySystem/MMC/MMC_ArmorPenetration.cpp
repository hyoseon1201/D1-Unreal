// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MMC/MMC_ArmorPenetration.h"

#include "AbilitySystem/D1AttributeSet.h"

UMMC_ArmorPenetration::UMMC_ArmorPenetration()
{
	DexterityDef.AttributeToCapture = UD1AttributeSet::GetDexterityAttribute();
	DexterityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	DexterityDef.bSnapshot = false;

	LuckDef.AttributeToCapture = UD1AttributeSet::GetLuckAttribute();
	LuckDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	LuckDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(DexterityDef);
	RelevantAttributesToCapture.Add(LuckDef);
}

float UMMC_ArmorPenetration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Dexterity = 0.f;
	GetCapturedAttributeMagnitude(DexterityDef, Spec, EvaluationParameters, Dexterity);
	Dexterity = FMath::Max<float>(Dexterity, 0.f);

	float Luck = 0.f;
	GetCapturedAttributeMagnitude(LuckDef, Spec, EvaluationParameters, Luck);
	Luck = FMath::Max<float>(Luck, 0.f);

	return (Dexterity * 1.0f) + (Luck * 0.5f);
}