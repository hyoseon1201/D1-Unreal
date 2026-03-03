// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "D1AbilitySystemConfig.generated.h"

/**
 * 
 */
UCLASS()
class D1_API UD1AbilitySystemConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// --- 전투 관련 전역 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UCurveTable> DamageCalculationCoefficients;
};
