// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeDungeon.h"
#include "D1/D1.h"
#include "Player/D1PlayerController.h"

void AD1GameModeDungeon::OnBossDefeated()
{
	UE_LOG(LogD1, Log, TEXT("Boss defeated! Notifying all players..."));

	// 획득 아이템 데이터 구성 (나중에 몬스터/보스 드랍 테이블에서 동적으로 가져올 예정)
	TArray<FText> LootItems;
	LootItems.Add(FText::FromString(TEXT("Sword of Flame")));
	LootItems.Add(FText::FromString(TEXT("Health Potion x3")));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AD1PlayerController* PC = Cast<AD1PlayerController>(*It))
		{
			PC->ClientShowDungeonResult(LootItems);
		}
	}
}
