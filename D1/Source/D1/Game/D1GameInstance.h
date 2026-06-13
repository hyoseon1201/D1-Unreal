// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Inventory/D1InventoryTypes.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include "D1GameInstance.generated.h"

/** 플레이어 한 명의 Travel 보존 데이터 */
USTRUCT()
struct FD1SavedPlayerData
{
	GENERATED_BODY()

	UPROPERTY() int32 AttributePoints = 0;
	UPROPERTY() int32 SkillPoints     = 0;
	UPROPERTY() int32 Level           = 1;
	UPROPERTY() int32 XP              = 0;

	UPROPERTY() float Strength        = 0.f;
	UPROPERTY() float Intelligence    = 0.f;
	UPROPERTY() float Dexterity       = 0.f;
	UPROPERTY() float Luck            = 0.f;

	UPROPERTY() TArray<FD1InventoryItem>    InventorySlots;
	UPROPERTY() TArray<FD1EquippedItem>     EquippedItems;
	UPROPERTY() TArray<FD1SavedAbilityInfo> AbilityStates;
};

/**
 * 맵 이동(ServerTravel) 시 PlayerState 데이터를 임시 저장/복원하는 GameInstance.
 * 플레이어별 슬롯(TMap)으로 관리하므로 멀티플레이어에서도 서로 덮어쓰지 않는다.
 */
UCLASS()
class D1_API UD1GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Travel 직전 서버 측에서 호출 — PlayerId 별로 저장 */
	void SavePlayerData(const FString& PlayerId, const FD1SavedPlayerData& Data);

	/** PossessedBy 에서 호출 — 저장된 데이터가 있으면 OutData에 복사하고 true 반환 */
	bool TryGetPlayerData(const FString& PlayerId, FD1SavedPlayerData& OutData) const;

	/** 복원 완료 후 슬롯 해제 */
	void ClearPlayerData(const FString& PlayerId);

	/** 전체 초기화 (레벨 언로드 등 예외 상황용) */
	UFUNCTION(BlueprintCallable, Category = "D1|Travel")
	void ClearAllSavedData();

	/** 특정 플레이어의 저장 데이터 유무 */
	bool HasSavedData(const FString& PlayerId) const { return SavedPlayers.Contains(PlayerId); }

private:
	/** Key = PartyPlayerId (UniqueNetId 문자열 or PlayerName) */
	UPROPERTY()
	TMap<FString, FD1SavedPlayerData> SavedPlayers;
};
