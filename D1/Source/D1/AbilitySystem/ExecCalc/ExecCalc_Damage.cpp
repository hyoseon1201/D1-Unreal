#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/Data/D1CharacterClassInfo.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "Interaction/CombatInterface.h"
#include "D1GameplayTags.h"
#include <D1AbilityTypes.h>
#include <AbilitySystem/Data/D1AbilitySystemConfig.h>

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

	// --- 3. 커브 테이블 데이터 추출 (Global Config 기반) ---
	// GetWorld()를 통해 전역 설정 에셋을 가져옵니다.
	UD1AbilitySystemConfig* Config = UD1AbilitySystemLibrary::GetAbilitySystemConfig(SourceAvatar);

	float Armor_K = 1000.f; // 기본값
	float Pen_Coeff = 0.15f; // 기본값

	if (Config)
	{
		UE_LOG(LogTemp, Log, TEXT("[ExecCalc] Success: Found AbilitySystemConfig."));

		if (Config->DamageCalculationCoefficients)
		{
			const FRealCurve* ArmorKCurve = Config->DamageCalculationCoefficients->FindCurve(FName("Armor.K_Value"), FString());
			const FRealCurve* PenCoeffCurve = Config->DamageCalculationCoefficients->FindCurve(FName("ArmorPen.Coeff"), FString());

			if (ArmorKCurve && PenCoeffCurve)
			{
				Armor_K = ArmorKCurve->Eval(TargetLevel);
				Pen_Coeff = PenCoeffCurve->Eval(SourceLevel);
				UE_LOG(LogTemp, Log, TEXT("[ExecCalc] Success: Extracted Curve Values. Armor_K: %.2f, Pen_Coeff: %.4f (at Levels S:%f, T:%f)"), Armor_K, Pen_Coeff, SourceLevel, TargetLevel);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[ExecCalc] Fail: Could not find specific Rows (Armor.K_Value or ArmorPen.Coeff) in CurveTable!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ExecCalc] Fail: DamageCalculationCoefficients CurveTable is NULL in Config."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ExecCalc] Fail: Unable to find AbilitySystemConfig! Check if assigned in GameMode."));
	}

	// --- 4. 데미지 및 상세 계산 ---
	float Damage = 0.f;

	for (FGameplayTag DamageTypeTag : FD1GameplayTags::Get().DamageTypes)
	{
		const float DamageTypeValue = Spec.GetSetByCallerMagnitude(DamageTypeTag, false, 0.f);
		Damage += DamageTypeValue;
	}

	UE_LOG(LogTemp, Warning, TEXT("--- Damage Calculation Details ---"));
	UE_LOG(LogTemp, Log, TEXT("Raw Accumulated Damage: %.2f"), Damage);

	// [방어력 계산 공식 적용]
	const float EffectiveArmorPen = ArmorPenetration / (1.f + (SourceLevel * Pen_Coeff));
	const float FinalArmor = FMath::Max<float>(0.f, Armor - EffectiveArmorPen);
	const float DamageMultiplier = Armor_K / (FinalArmor + Armor_K);

	float FinalDamage = Damage * DamageMultiplier;

	UE_LOG(LogTemp, Log, TEXT("Captured Armor: %.2f | Captured Pen: %.2f"), Armor, ArmorPenetration);
	UE_LOG(LogTemp, Log, TEXT("Effective Pen: %.2f | Final Armor: %.2f | Multiplier: %.4f"), EffectiveArmorPen, FinalArmor, DamageMultiplier);

	// [치명타 판정]
	const bool bIsCritical = FMath::FRand() * 100.f < CritChance;
	if (bIsCritical)
	{
		FinalDamage *= (CritDamage / 100.f);
		UE_LOG(LogTemp, Warning, TEXT("!!! CRITICAL HIT !!! Multiplier applied: %.2f"), CritDamage / 100.f);
	}

	// Context에 치명타 여부 저장 (UI 표시용)
	FGameplayEffectContextHandle EffectContextHandle = Spec.GetContext();
	UD1AbilitySystemLibrary::SetIsCriticalHit(EffectContextHandle, bIsCritical);

	UE_LOG(LogTemp, Warning, TEXT(">> FINAL DAMAGE TO APPLY: %.2f <<"), FinalDamage);
	UE_LOG(LogTemp, Warning, TEXT("----------------------------------"));

	// --- 5. 결과 출력 ---
	const FGameplayModifierEvaluatedData EvaluatedData(UD1AttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, FinalDamage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}