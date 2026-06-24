// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/D1Barrack.h"
#include "Characters/D1Enemy.h"
#include "Components/DrawSphereComponent.h"
#include "D1.h"

AD1Barrack::AD1Barrack()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SpawnRadiusVisualizer = CreateDefaultSubobject<UDrawSphereComponent>(TEXT("SpawnRadiusVisualizer"));
	SpawnRadiusVisualizer->SetupAttachment(RootComponent);
	SpawnRadiusVisualizer->SetSphereRadius(SpawnRadius);
	SpawnRadiusVisualizer->ShapeColor = FColor::Yellow;
	SpawnRadiusVisualizer->bDrawOnlyIfSelected = true;
}

#if WITH_EDITOR
void AD1Barrack::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (SpawnRadiusVisualizer)
	{
		SpawnRadiusVisualizer->SetSphereRadius(SpawnRadius);
	}
}
#endif

void AD1Barrack::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (!MonsterClass)
	{
		UE_LOG(LogD1, Error, TEXT("D1Barrack: MonsterClass가 설정되지 않았습니다. (%s)"), *GetName());
		return;
	}

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AD1Barrack::TrySpawnMonster, RespawnInterval, true);
}

void AD1Barrack::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AD1Barrack::TrySpawnMonster()
{
	// 죽었거나 소멸된 몬스터를 목록에서 정리
	SpawnedMonsters.RemoveAll([](const TWeakObjectPtr<AD1Enemy>& Monster)
	{
		return !Monster.IsValid();
	});

	if (SpawnedMonsters.Num() >= MaxMonsterCount)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const FVector2D RandomOffset2D = FMath::RandPointInCircle(SpawnRadius);
	const FVector SpawnLocation = Origin + FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.f);
	const FRotator SpawnRotation = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AD1Enemy* NewMonster = GetWorld()->SpawnActor<AD1Enemy>(MonsterClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (NewMonster)
	{
		SpawnedMonsters.Add(NewMonster);
		UE_LOG(LogD1, Verbose, TEXT("D1Barrack: %s 스폰 (현재 %d/%d)"), *NewMonster->GetName(), SpawnedMonsters.Num(), MaxMonsterCount);
	}
}
