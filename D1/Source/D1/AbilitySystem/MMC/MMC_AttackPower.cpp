// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MMC/MMC_AttackPower.h"

#include "AbilitySystem/D1AttributeSet.h"

UMMC_AttackPower::UMMC_AttackPower()
{
	StrengthDef.AttributeToCapture = UD1AttributeSet::GetStrengthAttribute();
	StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StrengthDef.bSnapshot = false;

	// 2. 이 MMC가 감시할 속성 리스트에 추가
	RelevantAttributesToCapture.Add(StrengthDef);
}

float UMMC_AttackPower::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 3. 태그 및 소스/타겟 설정 가져오기
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// 4. 힘(Strength) 값 캡처
	float Strength = 0.f;
	GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);
	Strength = FMath::Max<float>(Strength, 0.f);

	// 5. 레벨 가져오기 (Spec에서 레벨 정보를 가지고 있음)
	float Level = Spec.GetLevel();

	// 6. 최종 공식 적용: 힘 * 2 + 레벨 * 1
	return (Strength * 2.f) + (Level * 1.f);
}
