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

	/** 마지막 플레이어 퇴장 시 다음 파티를 위해 맵 리셋 */
	virtual void Logout(AController* Exiting) override;

	/** 부하 테스트용 봇 접속(bIsTestBotConnection)이면 네브메시 전체에서 랜덤 위치에 스폰, 아니면 기존 PlayerStartTag 로직 그대로 */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
