// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/D1GameStateBase.h"
#include "D1GameStateTown.generated.h"

class AD1PlayerController;

/** 파티 목록 변경 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPartiesChanged);

/** 파티 멤버 정보 (GameStateTown에 의해 관리) */
USTRUCT()
struct FD1PartyMemberInfo
{
	GENERATED_BODY()

	/** 플레이어 이름 (PlayerState->GetPlayerName) */
	UPROPERTY()
	FString PlayerName;

	/** 준비 완료 여부 */
	UPROPERTY()
	bool bIsReady = false;
};

/** 파티 정보 */
USTRUCT()
struct FD1PartyInfo
{
	GENERATED_BODY()

	/** 고유 파티 ID */
	UPROPERTY()
	int32 PartyId = INDEX_NONE;

	/** 파티장 플레이어 이름 */
	UPROPERTY()
	FString LeaderName;

	/** 파티원 목록 (리더 포함) */
	UPROPERTY()
	TArray<FD1PartyMemberInfo> Members;

	/** 선택된 던전 맵 이름 */
	UPROPERTY()
	FString SelectedDungeon = TEXT("Dungeon");

	/** 특정 플레이어가 이 파티에 속해있는가? */
	bool HasMember(const FString& InPlayerName) const
	{
		for (const auto& Member : Members)
		{
			if (Member.PlayerName == InPlayerName)
			{
				return true;
			}
		}
		return false;
	}

	/** 파티장인가? */
	bool IsLeader(const FString& InPlayerName) const
	{
		return LeaderName == InPlayerName;
	}

	/** 모든 멤버가 준비되었는가? (파티장 제외하고 체크하거나, 파티장도 포함 - 여기서는 전원 체크) */
	bool IsAllReady() const
	{
		for (const auto& Member : Members)
		{
			if (!Member.bIsReady)
			{
				return false;
			}
		}
		return true;
	}
};

/**
 * 마을 전용 GameState
 * 파티 목록 관리 및 던전 입장 조율
 */
UCLASS()
class D1_API AD1GameStateTown : public AD1GameStateBase
{
	GENERATED_BODY()

public:
	AD1GameStateTown();

	/** 파티 목록 (Replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_Parties)
	TArray<FD1PartyInfo> Parties;

	/** 파티 목록 변경 시 클라이언트 UI 갱신 */
	UFUNCTION()
	void OnRep_Parties();

	/** 새로운 파티 생성 (서버 전용). 성공 여부 반환. */
	UFUNCTION(BlueprintCallable, Category = "Party", meta = (CallInEditor = "false"))
	bool ServerCreateParty(const FString& LeaderName);

	/** 파티 해산 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Party", meta = (CallInEditor = "false"))
	bool ServerDisbandParty(int32 PartyId);

	/** 파티에 멤버 추가 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Party", meta = (CallInEditor = "false"))
	bool ServerJoinParty(int32 PartyId, const FString& PlayerName);

	/** 파티에서 멤버 제거 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Party", meta = (CallInEditor = "false"))
	bool ServerLeaveParty(int32 PartyId, const FString& PlayerName);

	/** 준비 상태 토글 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Party", meta = (CallInEditor = "false"))
	bool ServerSetReady(int32 PartyId, const FString& PlayerName, bool bReady);

	/** 던전 선택 (파티장 전용, 서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Party", meta = (CallInEditor = "false"))
	bool ServerSelectDungeon(int32 PartyId, const FString& DungeonMap);

	/** 플레이어가 속한 파티 찾기 (C++ 전용) */
	FD1PartyInfo* FindPartyByPlayer(const FString& PlayerName);

	/** 플레이어가 속한 파티 찾기 (const 버전, C++ 전용) */
	const FD1PartyInfo* FindPartyByPlayer(const FString& PlayerName) const;

	/** 다음 사용할 파티 ID 생성 (서버 전용) */
	int32 GeneratePartyId();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 파티 목록이 변경되었을 때 브로드캐스트할 델리게이트 (블루프린트 바인딩용) */
	UPROPERTY(BlueprintAssignable, Category = "Party")
	FOnPartiesChanged OnPartiesChangedDelegate;

	/** 파티 ID 생성용 카운터 */
	int32 NextPartyId = 1;
};
