// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameInstance.h"
#include "D1/D1.h"
#include "GameFramework/GameNetworkManager.h"

void UD1GameInstance::Init()
{
	Super::Init();

	// 엔진 기본값(0.0166 ≈ 60Hz)은 서버 틱(30Hz)의 2배 빈도로 이동을 전송하게 만들어 대역폭 낭비
	// (Net Insights 실측: 60Hz→30Hz로 -58.6%, Docs/NetworkPerfOptimization_2026-06-26.md 6-4장) → 서버 틱에 맞춤
	if (AGameNetworkManager* NetManager = GetMutableDefault<AGameNetworkManager>())
	{
		NetManager->ClientNetSendMoveDeltaTime = 0.033333f;
		NetManager->ClientNetSendMoveDeltaTimeThrottled = 0.033333f;
	}
}

void UD1GameInstance::SavePlayerData(const FString& PlayerId, const FD1SavedPlayerData& Data)
{
	SavedPlayers.Add(PlayerId, Data);

	UE_LOG(LogD1Travel, Warning,
		TEXT("GameInstance::SavePlayerData [%s]: Level=%d, AttrPts=%d, SkillPts=%d, Str=%.1f, Inventory=%d, Equipped=%d, Abilities=%d"),
		*PlayerId, Data.Level, Data.AttributePoints, Data.SkillPoints, Data.Strength,
		Data.InventorySlots.Num(), Data.EquippedItems.Num(), Data.AbilityStates.Num());
}

bool UD1GameInstance::TryGetPlayerData(const FString& PlayerId, FD1SavedPlayerData& OutData) const
{
	if (const FD1SavedPlayerData* Found = SavedPlayers.Find(PlayerId))
	{
		OutData = *Found;
		return true;
	}
	return false;
}

void UD1GameInstance::ClearPlayerData(const FString& PlayerId)
{
	SavedPlayers.Remove(PlayerId);
	UE_LOG(LogD1Travel, Warning, TEXT("GameInstance::ClearPlayerData [%s]"), *PlayerId);
}

void UD1GameInstance::ClearAllSavedData()
{
	SavedPlayers.Empty();
	UE_LOG(LogD1Travel, Warning, TEXT("GameInstance::ClearAllSavedData"));
}
