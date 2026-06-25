// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "D1GameModeBase.generated.h"

class UD1CharacterClassInfo;
class UD1AbilitySystemConfig;
class UD1AbilityInfo;
class UD1ItemRegistry;
class AD1GameStateBase;
class AD1PlayerState;

/**
 * 
 */
UCLASS()
class D1_API AD1GameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AD1GameModeBase();

	virtual void PostInitializeComponents() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void BeginPlay() override;

	/** 부하 테스트용 봇 접속(bIsTestBotConnection)이면 네브메시 전체에서 랜덤 위치에 스폰, 아니면 기존 PlayerStartTag 로직 그대로 */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** 접속 URL 옵션(?sessionToken=)을 파싱해 PlayerState에 보관 */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal = TEXT("")) override;

	/** 접속 종료(정상/타임아웃) 시 캐릭터 데이터를 웹서버에 저장 */
	virtual void Logout(AController* Exiting) override;

	/** 이 GameMode에서 Ability 사용(전투/버프/회복 등)을 허용하는가? */
	UFUNCTION(BlueprintPure, Category = "D1|GameMode Rules")
	virtual bool AreAbilitiesAllowed() const { return true; }

	UPROPERTY(EditDefaultsOnly, Category = "Characer Class Defaults")
	TObjectPtr<UD1CharacterClassInfo> CharacterClassInfo;

	UPROPERTY(EditDefaultsOnly, Category = "Characer Class Defaults")
	TObjectPtr<UD1AbilitySystemConfig> AbilitySystemConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Info")
	TObjectPtr<UD1AbilityInfo> AbilityInfo;

	UPROPERTY(EditDefaultsOnly, Category = "Item Data")
	TObjectPtr<UD1ItemRegistry> ItemRegistry;

private:
	/** 현재 월드의 살아있는 Enemy 수를 세서 로그로 찍고, 누적 평균을 갱신한다 (부하 테스트용). 몬스터가 없는 맵(Town 등)에서는 로그를 남기지 않는다. */
	void LogMonsterCount();

	FTimerHandle MonsterCountTimerHandle;

	/** Enemy 수 로깅 주기 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "D1|Debug")
	float MonsterCountLogInterval = 5.f;

	// 누적 평균 계산용 (Enemy가 1마리 이상인 샘플만 집계)
	int64 MonsterCountSum = 0;
	int32 MonsterCountSamples = 0;

	// 테스트 봇을 Bot1, Bot2... 태그 PlayerStart에 순서대로 배정하기 위한 카운터
	int32 TestBotSpawnIndex = 0;
};
