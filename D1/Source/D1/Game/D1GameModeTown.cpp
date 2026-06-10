// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeTown.h"
#include "D1/D1.h"
#include "Game/D1GameStateTown.h"
#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"
#include "GameFramework/PlayerState.h"

AD1GameModeTown::AD1GameModeTown()
{
	GameStateClass = AD1GameStateTown::StaticClass();
}

AD1GameStateTown* AD1GameModeTown::GetGameStateTown() const
{
	return Cast<AD1GameStateTown>(GameState);
}

void AD1GameModeTown::StartDungeonForParty(AD1PlayerController* LeaderPC)
{
	if (!HasAuthority() || !LeaderPC)
	{
		return;
	}

	AD1PlayerState* LeaderPS = LeaderPC->GetPlayerState<AD1PlayerState>();
	if (!LeaderPS)
	{
		UE_LOG(LogD1Party, Warning, TEXT("StartDungeon: Leader has no PlayerState"));
		return;
	}

	AD1GameStateTown* GSTown = GetGameStateTown();
	if (!GSTown)
	{
		UE_LOG(LogD1Party, Error, TEXT("StartDungeon: GameStateTown is NULL!"));
		return;
	}

	const FString LeaderId = LeaderPS->GetPartyPlayerId();
	const FString LeaderName = LeaderPS->GetPlayerName();
	FD1PartyInfo* Party = GSTown->FindPartyByPlayerId(LeaderId);
	if (!Party)
	{
		UE_LOG(LogD1Party, Warning, TEXT("StartDungeon: %s is not in a party."), *LeaderName);
		return;
	}

	if (!Party->IsLeader(LeaderId))
	{
		UE_LOG(LogD1Party, Warning, TEXT("StartDungeon: %s is not the party leader."), *LeaderName);
		return;
	}

	// MVP: 준비 체크 스킵 가능 (주석 해제하면 활성화)
	/*
	if (!Party->IsAllReady())
	{
		UE_LOG(LogD1Party, Warning, TEXT("StartDungeon: Not all members are ready."));
		return;
	}
	*/

	const FString TargetDungeon = Party->SelectedDungeon;
	UE_LOG(LogD1Party, Log, TEXT("StartDungeon: PartyId=%d, Leader=%s, Members=%d, Dungeon=%s"),
		Party->PartyId, *LeaderName, Party->Members.Num(), *TargetDungeon);

	// MVP: 단일 서버이므로 ServerTravel로 접속자 전원을 함께 이동시킨다.
	// (주의) 파티원만 골라서 이동시키는 것은 단일 서버에서 불가능 — 마을에 남은 다른 파티도 함께 끌려감.
	//
	// Phase 3 (웹서버 연동 시) 교체 예정:
	//   1. 웹서버에 던전 인스턴스 생성 요청 → 별도 데디서버 프로세스 기동 (IP:Port 발급)
	//   2. 파티원 각각의 캐릭터 데이터를 웹서버/DB에 저장 (크로스 프로세스라 GameInstance 사용 불가)
	//   3. 파티원에게만 ClientTravel("IP:Port")로 새 던전 서버에 직접 접속시킴
	//      (ClientTravel에 맵 이름을 주면 로컬 오프라인 월드가 열리지만, 주소를 주면 해당 서버 접속이므로 정상)
	GetWorld()->ServerTravel(TargetDungeon);
}

void AD1GameModeTown::ShowLoadingForPartyMembers(const TArray<FString>& MemberNames)
{
	// TODO: Client RPC로 각 멤버에게 로딩 화면 표시 지시
	// 현재는 TravelToMap 자체가 로딩을 유발하므로 별도 구현은 Phase 2에서
}

