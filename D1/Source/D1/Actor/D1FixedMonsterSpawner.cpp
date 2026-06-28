// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/D1FixedMonsterSpawner.h"
#include "Characters/D1Enemy.h"
#include "Components/DrawSphereComponent.h"
#include "DrawDebugHelpers.h"
#include "D1.h"

AD1FixedMonsterSpawner::AD1FixedMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SpawnAreaVisualizer = CreateDefaultSubobject<UDrawSphereComponent>(TEXT("SpawnAreaVisualizer"));
	SpawnAreaVisualizer->SetupAttachment(RootComponent);
	SpawnAreaVisualizer->SetSphereRadius(SpawnAreaRadius);
	SpawnAreaVisualizer->ShapeColor = FColor::Cyan;
	SpawnAreaVisualizer->bDrawOnlyIfSelected = true;
}

void AD1FixedMonsterSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SpawnAreaVisualizer)
	{
		SpawnAreaVisualizer->SetSphereRadius(SpawnAreaRadius);
	}

	DrawSpawnPointDebugVisuals();
}

void AD1FixedMonsterSpawner::DrawSpawnPointDebugVisuals() const
{
	if (!GetWorld())
	{
		return;
	}

	// 순수 에디터 뷰포트(Play 누르기 전)에서만 그림 — PIE/패키징 게임/데디서버 플레이 중에는 표시 안 함
	if (GetWorld()->WorldType != EWorldType::Editor)
	{
		return;
	}

	for (const FVector& Point : SpawnPoints)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation() + Point, SpawnPointVisualRadius, 8, SpawnPointVisualColor, true, -1.f, 0, 1.5f);
	}
}

#if WITH_EDITOR
void AD1FixedMonsterSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (SpawnAreaVisualizer)
	{
		SpawnAreaVisualizer->SetSphereRadius(SpawnAreaRadius);
	}
}

void AD1FixedMonsterSpawner::GenerateSpawnPoints()
{
	// CallInEditor 버튼으로 직접 프로퍼티를 바꾸면 디테일 패널 편집과 달리 자동으로 dirty 표시가 안 됨.
	// Modify() 없이는 레벨이 "수정됨" 표시가 안 돼서 Save All 시 누락되고, 패키징 시 SpawnPoints가 빈 채로 구워짐.
	Modify();

	SpawnPoints.Empty();

	constexpr int32 MaxAttemptsPerPoint = 30;
	const float MinDistSq = FMath::Square(MinPointDistance);

	for (int32 i = 0; i < NumSpawnPoints; ++i)
	{
		bool bPlaced = false;

		for (int32 Attempt = 0; Attempt < MaxAttemptsPerPoint; ++Attempt)
		{
			const FVector2D Candidate2D = FMath::RandPointInCircle(SpawnAreaRadius);
			const FVector Candidate(Candidate2D.X, Candidate2D.Y, 0.f);

			bool bFarEnough = true;
			for (const FVector& Existing : SpawnPoints)
			{
				if (FVector::DistSquared(Candidate, Existing) < MinDistSq)
				{
					bFarEnough = false;
					break;
				}
			}

			if (bFarEnough)
			{
				SpawnPoints.Add(Candidate);
				bPlaced = true;
				break;
			}
		}

		// 최소 거리 조건으로 30번 시도해도 못 찾으면, 조건 무시하고 일단 배치(전체 개수는 보장)
		if (!bPlaced)
		{
			const FVector2D Candidate2D = FMath::RandPointInCircle(SpawnAreaRadius);
			SpawnPoints.Add(FVector(Candidate2D.X, Candidate2D.Y, 0.f));
			UE_LOG(LogD1, Warning, TEXT("D1FixedMonsterSpawner: %d번째 포인트는 최소거리(%.0f) 조건을 못 맞춰 무시하고 배치"), i, MinPointDistance);
		}
	}

	UE_LOG(LogD1, Log, TEXT("D1FixedMonsterSpawner: 스폰 포인트 %d개 생성 완료 (%s)"), SpawnPoints.Num(), *GetName());

	FlushPersistentDebugLines(GetWorld());
	DrawSpawnPointDebugVisuals();
}
#endif

void AD1FixedMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (!MonsterClass)
	{
		UE_LOG(LogD1, Error, TEXT("D1FixedMonsterSpawner: MonsterClass가 설정되지 않았습니다. (%s)"), *GetName());
		return;
	}

	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(LogD1, Error, TEXT("D1FixedMonsterSpawner: SpawnPoints가 비어있습니다. 에디터에서 GenerateSpawnPoints를 먼저 실행하고 저장하세요. (%s)"), *GetName());
		return;
	}

	SlotMonsters.SetNum(SpawnPoints.Num());

	// 시작 시 모든 포인트에 한 번에 스폰 (랜덤 점진적 증가가 아니라 즉시 정원 채움)
	for (int32 i = 0; i < SpawnPoints.Num(); ++i)
	{
		SpawnAtSlot(i);
	}

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AD1FixedMonsterSpawner::CheckAndRespawn, RespawnCheckInterval, true);
}

void AD1FixedMonsterSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AD1FixedMonsterSpawner::CheckAndRespawn()
{
	// 한 틱에 처리할 스폰 수를 제한 — 동시에 여럭 죽어 빈 슬롯이 몰려도, 그만큼을 한 프레임에
	// 다 스폰하면 SpawnActor 비용이 몰려 스파이크가 됨. 나머지는 다음 호출(RespawnCheckInterval 뒤)로 미룸.
	int32 SpawnedThisCheck = 0;

	for (int32 i = 0; i < SlotMonsters.Num() && SpawnedThisCheck < MaxRespawnsPerCheck; ++i)
	{
		if (!SlotMonsters[i].IsValid())
		{
			SpawnAtSlot(i);
			++SpawnedThisCheck;
		}
	}
}

void AD1FixedMonsterSpawner::SpawnAtSlot(int32 SlotIndex)
{
	const FVector SpawnLocation = GetActorLocation() + SpawnPoints[SlotIndex];
	const FRotator SpawnRotation(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AD1Enemy* NewMonster = GetWorld()->SpawnActor<AD1Enemy>(MonsterClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (NewMonster)
	{
		SlotMonsters[SlotIndex] = NewMonster;
		UE_LOG(LogD1, Verbose, TEXT("D1FixedMonsterSpawner: 슬롯 %d에 %s 스폰 (위치 %s)"), SlotIndex, *NewMonster->GetName(), *SpawnLocation.ToString());
	}
}
