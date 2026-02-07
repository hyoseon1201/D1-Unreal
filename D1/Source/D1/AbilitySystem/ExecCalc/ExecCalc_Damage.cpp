#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/Data/D1CharacterClassInfo.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "Interaction/CombatInterface.h"
#include "D1GameplayTags.h"
#include <D1AbilityTypes.h>

struct D1DamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorPenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitDamage);

	D1DamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UD1AttributeSet, Armor, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UD1AttributeSet, ArmorPenetration, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UD1AttributeSet, CriticalHitChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UD1AttributeSet, CriticalHitDamage, Source, false);
	}
};

static const D1DamageStatics& DamageStatics()
{
	static D1DamageStatics DStatics;
	return DStatics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitDamageDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// --- 1. 기초 정보 및 인터페이스 추출 ---
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceAvatar = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	float SourceLevel = 1.f;
	if (SourceAvatar && SourceAvatar->Implements<UCombatInterface>())
	{
		SourceLevel = ICombatInterface::Execute_GetPlayerLevel(SourceAvatar);
	}

	float TargetLevel = 1.f;
	if (TargetAvatar && TargetAvatar->Implements<UCombatInterface>())
	{
		TargetLevel = ICombatInterface::Execute_GetPlayerLevel(TargetAvatar);
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// --- 2. 속성 캡처 (Attributes Capture) ---
	float Armor = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluationParameters, Armor);

	float ArmorPenetration = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationDef, EvaluationParameters, ArmorPenetration);

	float CritChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitChanceDef, EvaluationParameters, CritChance);

	float CritDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitDamageDef, EvaluationParameters, CritDamage);

	// --- 3. 커브 테이블 데이터 추출 ---
	UD1CharacterClassInfo* CharacterClassInfo = UD1AbilitySystemLibrary::GetCharacterClassInfo(SourceAvatar);
	float Armor_K = 1000.f;
	float Pen_Coeff = 0.15f;

	if (CharacterClassInfo && CharacterClassInfo->DamageCalculationCoefficients)
	{
		const FRealCurve* ArmorKCurve = CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("Armor.K_Value"), FString());
		const FRealCurve* PenCoeffCurve = CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("ArmorPen.Coeff"), FString());

		if (ArmorKCurve) Armor_K = ArmorKCurve->Eval(TargetLevel);
		if (PenCoeffCurve) Pen_Coeff = PenCoeffCurve->Eval(SourceLevel);
	}

	// --- 4. 데미지 및 치명타 계산 ---
	float Damage = 0.f;

	for (FGameplayTag DamageTypeTag : FD1GameplayTags::Get().DamageTypes)
	{
		// 세 번째 인자로 0.f를 주어 태그가 없을 경우 0을 반환하게 합니다.
		const float DamageTypeValue = Spec.GetSetByCallerMagnitude(DamageTypeTag, false, 0.f);

		if (DamageTypeValue > 0.f)
		{
			UE_LOG(LogTemp, Log, TEXT("Damage Type: %s | Value: %.2f"), *DamageTypeTag.ToString(), DamageTypeValue);
		}

		Damage += DamageTypeValue;
	}

	UE_LOG(LogTemp, Warning, TEXT("Total Raw Damage: %.2f"), Damage);

	// [방어력 계산]
	const float EffectiveArmorPen = ArmorPenetration / (1.f + (SourceLevel * Pen_Coeff));
	const float FinalArmor = FMath::Max<float>(0.f, Armor - EffectiveArmorPen);
	const float DamageMultiplier = Armor_K / (FinalArmor + Armor_K);

	float FinalDamage = Damage * DamageMultiplier;

	// [치명타 판정]
	// FRand()는 0~1 사이의 값을 반환합니다. CritChance는 0~100 사이의 % 수치라고 가정합니다.
	const bool bIsCritical = FMath::FRand() * 100.f < CritChance;

	if (bIsCritical)
	{
		// CritDamage가 150이라면 1.5배 데미지
		FinalDamage *= (CritDamage / 100.f);
	}

	FGameplayEffectContextHandle EffectContextHandle = Spec.GetContext();
	FGameplayEffectContext* Context = EffectContextHandle.Get();
	FD1GameplayEffectContext* D1Context = static_cast<FD1GameplayEffectContext*>(Context);
	UD1AbilitySystemLibrary::SetIsCriticalHit(EffectContextHandle, bIsCritical);

	// --- 5. 결과 출력 및 로그 ---
	const FGameplayModifierEvaluatedData EvaluatedData(UD1AttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, FinalDamage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}