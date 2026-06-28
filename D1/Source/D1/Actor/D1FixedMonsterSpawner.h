// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "D1FixedMonsterSpawner.generated.h"

class AD1Enemy;
class UDrawSphereComponent;

/**
 * 고정 스폰 포인트 기반 몬스터 스포너 (전통적 RPG 스폰 슬롯 패턴).
 * 에디터에서 GenerateSpawnPoints로 골고루 분포된 위치 N개를 한 번 생성해 SpawnPoints에 저장(고정값으로 베이크).
 * BeginPlay 시 모든 포인트에 한 번에 스폰하고, 이후 주기적으로 각 포인트를 확인해
 * 그 포인트의 몬스터가 죽어있으면 정확히 같은 위치에 재스폰한다.
 * D1Barrack(반경 내 랜덤 스폰)과 달리 매 측정마다 분포가 달라지지 않아 부하테스트 재현성이 좋다.
 * 서버 권한에서만 동작.
 */
UCLASS()
class D1_API AD1FixedMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AD1FixedMonsterSpawner();

#if WITH_EDITOR
	/** SpawnAreaRadius 내에 NumSpawnPoints개를 MinPointDistance 이상 떨어뜨려 랜덤 생성, SpawnPoints에 저장(에디터에서 1회 실행 후 저장) */
	UFUNCTION(CallInEditor, Category = "D1|Spawn")
	void GenerateSpawnPoints();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** 저장된 SpawnPoints를 디버그 스피어로 다시 그린다 (레벨 로드/액터 이동/생성 시마다 호출돼 시각화가 항상 최신 상태를 반영) */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** SpawnPoints 배열 내용을 디버그 스피어로 그림 */
	void DrawSpawnPointDebugVisuals() const;

	/** 모든 스폰 포인트를 순회하며, 슬롯이 비어있으면(몬스터 사망) 그 위치에 재스폰 */
	void CheckAndRespawn();

	void SpawnAtSlot(int32 SlotIndex);

	/** 스폰할 몬스터 클래스 */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	TSubclassOf<AD1Enemy> MonsterClass;

	/** 생성할 스폰 포인트 수 (= 항상 유지되는 최대 동시 몬스터 수) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	int32 NumSpawnPoints = 99;

	/** 스폰 포인트를 생성할 반경 (액터 위치 기준 수평) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	float SpawnAreaRadius = 5000.f;

	/** 포인트끼리 유지할 최소 거리 (골고루 분포되도록 — 너무 작으면 뭉치고, 너무 크면 다 못 배치될 수 있음) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	float MinPointDistance = 300.f;

	/** 빈 슬롯(사망한 자리)을 재확인하는 주기 (초) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	float RespawnCheckInterval = 10.f;

	/** 한 번의 CheckAndRespawn 호출(틱)에서 실제로 스폰을 수행할 최대 개수.
	 * 동시에 여러 마리가 죽으면 빈 슬롯도 한꺼번에 몰리는데, 그걸 다 한 틱에 스폰하면
	 * SpawnActor(GAS 초기화+AI 컨트롤러 possess 등) 비용이 그 프레임에 몰려 스파이크가 됨.
	 * 이 값으로 한 틱당 처리량을 제한해 여러 틱에 걸쳐 분산시킨다. */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	int32 MaxRespawnsPerCheck = 5;

	/** 한 번 생성되면 고정값으로 저장되는 스폰 위치 목록 (액터 위치 기준 로컬 오프셋, Z=0) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn")
	TArray<FVector> SpawnPoints;

	/** 개별 스폰 포인트 디버그 스피어 반경 */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn|Debug")
	float SpawnPointVisualRadius = 80.f;

	/** 개별 스폰 포인트 디버그 스피어 색상 (같은 레벨에 스포너 여러 개 둘 때 구분용으로 유용) */
	UPROPERTY(EditAnywhere, Category = "D1|Spawn|Debug")
	FColor SpawnPointVisualColor = FColor::Yellow;

private:
	/** 에디터 뷰포트에서 SpawnAreaRadius를 와이어프레임 구체로 시각화 (Play 안 해도 보임) */
	UPROPERTY(VisibleAnywhere, Transient)
	TObjectPtr<UDrawSphereComponent> SpawnAreaVisualizer;

	/** SpawnPoints와 인덱스 1:1로 매칭되는 현재 슬롯의 몬스터. Invalid면 그 슬롯은 비어있음(=사망) */
	UPROPERTY()
	TArray<TWeakObjectPtr<AD1Enemy>> SlotMonsters;

	FTimerHandle RespawnTimerHandle;
};
