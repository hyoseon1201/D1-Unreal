// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeDungeon.h"
#include "D1/D1.h"
#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"
#include "Game/D1HttpSubsystem.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "GameFramework/PlayerStart.h"

void AD1GameModeDungeon::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Super 호출 후 남은 플레이어 수 확인 (Exiting은 이미 목록에서 제거됨)
	int32 RemainingCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->IsValid())
		{
			++RemainingCount;
		}
	}

	if (RemainingCount == 0)
	{
		UE_LOG(LogD1Travel, Log, TEXT("던전 마지막 플레이어 퇴장 — 다음 파티를 위해 맵 리셋: %s"),
			*GetWorld()->URL.Map);
		GetWorld()->ServerTravel(GetWorld()->URL.Map);
	}
}

void AD1GameModeDungeon::OnBossDefeated()
{
	UE_LOG(LogD1, Log, TEXT("Boss defeated! 결과창 표시 + 인벤토리 체크포인트 저장"));

	UD1HttpSubsystem* Http = GetGameInstance() ? GetGameInstance()->GetSubsystem<UD1HttpSubsystem>() : nullptr;

	TArray<FText> LootItems;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AD1PlayerController* PC = Cast<AD1PlayerController>(It->Get());
		if (!PC) continue;

		// 체크포인트 저장 (드롭 아이템이 인벤토리에 반영된 직후 시점)
		// ReturnToTown 시에도 저장하지만, 그 전에 크래시 대비
		if (Http)
		{
			if (AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>())
			{
				if (PS->WebCharacterId > 0)
				{
					Http->SaveCharacter(PS->WebCharacterId, PS->BuildSaveJson());
					UE_LOG(LogD1Inventory, Log, TEXT("OnBossDefeated: 체크포인트 저장 — CharId=%lld"), PS->WebCharacterId);
				}
			}
		}

		PC->ClientShowDungeonResult(LootItems);
	}
}

AActor* AD1GameModeDungeon::ChoosePlayerStart_Implementation(AController* Player)
{
	const AD1PlayerState* PS = Player ? Player->GetPlayerState<AD1PlayerState>() : nullptr;
	if (PS && PS->bIsTestBotConnection)
	{
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
		{
			// GetRandomPoint가 가끔 실패하는 걸 대비해 몇 번 재시도
			constexpr int32 MaxAttempts = 5;
			for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
			{
				FNavLocation RandomLoc;
				if (NavSys->GetRandomPoint(RandomLoc))
				{
					APlayerStart* TempStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), RandomLoc.Location, FRotator::ZeroRotator);
					if (TempStart)
					{
						TempStart->SetLifeSpan(1.f);
						UE_LOG(LogD1Travel, Log, TEXT("ChoosePlayerStart: 테스트 봇 랜덤 스폰 → %s (시도 %d/%d)"), *RandomLoc.Location.ToString(), Attempt + 1, MaxAttempts);
						return TempStart;
					}
				}
			}
			UE_LOG(LogD1Travel, Warning, TEXT("ChoosePlayerStart: 테스트 봇 랜덤 위치 탐색 %d회 모두 실패, 기본 로직으로 폴백"), MaxAttempts);
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}
