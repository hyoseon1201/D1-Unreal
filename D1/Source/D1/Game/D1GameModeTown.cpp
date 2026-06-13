// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeTown.h"
#include "D1/D1.h"
#include "Game/D1GameStateTown.h"
#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

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

	// ServerTravel 전에 모든 플레이어 데이터를 저장 (PlayerState가 아직 유효한 시점)
	UWorld* World = GetWorld();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AD1PlayerController* PC = Cast<AD1PlayerController>(It->Get()))
		{
			PC->SaveTravelDataToGameInstance();
		}
	}

	// 짧은 맵 이름이면 전체 경로로 변환 (패키징 빌드에서 짧은 이름은 찾지 못함)
	FString TravelURL = TargetDungeon;
	if (!TravelURL.StartsWith(TEXT("/")))
	{
		TravelURL = FString::Printf(TEXT("/Game/Maps/%s"), *TargetDungeon);
	}

	UE_LOG(LogD1Party, Log, TEXT("StartDungeon: ServerTravel → %s"), *TravelURL);
	World->ServerTravel(TravelURL);
}

void AD1GameModeTown::ShowLoadingForPartyMembers(const TArray<FString>& MemberNames)
{
	// TODO: Client RPC로 각 멤버에게 로딩 화면 표시 지시
	// 현재는 TravelToMap 자체가 로딩을 유발하므로 별도 구현은 Phase 2에서
}

