// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/D1GameModeBase.h"
#include "D1GameModeHuntingGround.generated.h"

/**
 * 사냥터(HuntingGround) 전용 GameMode.
 * 던전(AD1GameModeDungeon)과 달리 보스 처치 결과창/마지막 퇴장 시 맵 리셋 같은 던전 전용 규칙이 없는,
 * 상시 운영형 전투 맵. 봇 랜덤 스폰·몬스터 수 로깅 등 공통 기능은 AD1GameModeBase에서 상속받는다.
 * 사냥터 고유 규칙이 생기면 여기에 추가한다.
 */
UCLASS()
class D1_API AD1GameModeHuntingGround : public AD1GameModeBase
{
	GENERATED_BODY()
};
