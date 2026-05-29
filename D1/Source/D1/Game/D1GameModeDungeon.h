// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/D1GameModeBase.h"
#include "D1GameModeDungeon.generated.h"

/**
 * 
 */
UCLASS()
class D1_API AD1GameModeDungeon : public AD1GameModeBase
{
	GENERATED_BODY()

public:
	/** 보스가 사망하면 모든 플레이어에게 결과 위젯을 띄움 */
	UFUNCTION(BlueprintCallable, Category = "D1|Dungeon")
	void OnBossDefeated();
};
