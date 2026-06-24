// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "D1Barrack.generated.h"

class AD1Enemy;
class UDrawSphereComponent;

/**
 * 일정 시간마다 몬스터를 자동 스폰하는 액터.
 * 현재 살아있는 몬스터 수가 MaxMonsterCount 이상이면 리스폰을 멈춘다.
 * 서버 권한에서만 동작.
 */
UCLASS()
class D1_API AD1Barrack : public AActor
{
	GENERATED_BODY()

public:
	AD1Barrack();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 죽은 몬스터를 목록에서 정리하고, 빈 자리가 있으면 한 마리 스폰한다 */
	void TrySpawnMonster();

	/** 스폰할 몬스터 클래스 */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	TSubclassOf<AD1Enemy> MonsterClass;

	/** 동시에 살아있을 수 있는 최대 몬스터 수. 이 수를 넘으면 리스폰 대기 */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	int32 MaxMonsterCount = 5;

	/** 리스폰 시도 간격 (초) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	float RespawnInterval = 10.f;

	/** 액터 위치를 중심으로 스폰될 반경 (수평) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	float SpawnRadius = 500.f;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AD1Enemy>> SpawnedMonsters;

	FTimerHandle RespawnTimerHandle;

	/** 에디터 뷰포트에서 SpawnRadius를 와이어프레임 구체로 시각화 (Play 안 해도 보임). WITH_EDITORONLY_DATA로 감싸면 안 됨 — 서버 쿡 빌드에서 컴포넌트가 사라져 BP 로드가 깨짐 */
	UPROPERTY(VisibleAnywhere, Transient)
	TObjectPtr<UDrawSphereComponent> SpawnRadiusVisualizer;
};
