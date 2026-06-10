// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "Game/D1GameStateTown.h"
#include "D1DungeonPartyWidgetController.generated.h"

class AD1GameStateTown;
class AD1PlayerController;

/** 파티 목록 전체가 변경될 때 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartiesUpdated, const TArray<FD1PartyInfo>&, Parties);

/**
 * 마을 파티 UI(WBP_DungeonParty)의 데이터를 관리하는 WidgetController.
 *
 * - GameStateTown.OnPartiesChangedDelegate 를 바인딩하여 파티 목록 변경을 감지
 * - Blueprint 위젯에서 파티 생성/참가/탈퇴/준비/던전 선택/시작을 호출할 수 있는 인터페이스 제공
 * - 실제 서버 처리는 D1PlayerController Server RPC → D1GameStateTown 함수로 위임
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1DungeonPartyWidgetController : public UD1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	// ─── Delegates (Blueprint 바인딩용) ───────────────────────────────────

	/** 파티 목록이 변경될 때마다 전체 목록을 브로드캐스트 */
	UPROPERTY(BlueprintAssignable, Category = "Party")
	FOnPartiesUpdated OnPartiesUpdated;

	// ─── 파티 액션 (Blueprint 버튼에서 호출) ─────────────────────────────

	/** 새 파티 생성 (본인이 파티장). 생성 시점에 선택된 던전 맵 이름과 방 제목을 함께 전달 */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void CreateParty(const FString& DungeonMap, const FString& PartyName);

	/** 특정 파티에 참가 */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void JoinParty(int32 PartyId);

	/** 현재 파티에서 탈퇴 (파티장이면 파티 해산) */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void LeaveParty();

	/** 준비 상태 토글 */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void SetReady(bool bReady);

	/** 던전 맵 선택 (파티장 전용) */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void SelectDungeon(const FString& DungeonMap);

	/** 던전 입장 (파티장 전용, 모든 파티원 이동) */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void StartDungeon();

	// ─── 쿼리 (Blueprint Pure) ───────────────────────────────────────────

	/** 현재 로컬 플레이어가 파티에 속해있는가? */
	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsInParty() const;

	/** 현재 로컬 플레이어가 파티장인가? */
	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsPartyLeader() const;

	/**
	 * 현재 로컬 플레이어가 속한 파티 정보 반환.
	 * 파티에 속하지 않으면 PartyId == INDEX_NONE 인 빈 구조체 반환.
	 */
	UFUNCTION(BlueprintPure, Category = "Party")
	FD1PartyInfo GetMyParty() const;

private:
	/** GameStateTown.OnPartiesChangedDelegate 콜백 → OnPartiesUpdated 브로드캐스트 */
	UFUNCTION()
	void HandlePartiesChanged();

	/** 현재 World의 GameStateTown 캐스팅 헬퍼 */
	AD1GameStateTown* GetGameStateTown() const;

	/** 로컬 플레이어 고유 식별자 헬퍼 (PlayerState->GetPartyPlayerId) */
	FString GetLocalPlayerId() const;
};
