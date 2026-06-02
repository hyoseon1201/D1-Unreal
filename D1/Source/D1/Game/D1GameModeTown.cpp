// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeTown.h"
#include "Game/D1GameStateTown.h"
#include "Player/D1PlayerController.h"
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

	APlayerState* LeaderPS = LeaderPC->GetPlayerState<APlayerState>();
	if (!LeaderPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartDungeon] Leader has no PlayerState"));
		return;
	}

	AD1GameStateTown* GSTown = GetGameStateTown();
	if (!GSTown)
	{
		UE_LOG(LogTemp, Error, TEXT("[StartDungeon] GameStateTown is NULL!"));
		return;
	}

	const FString LeaderName = LeaderPS->GetPlayerName();
	FD1PartyInfo* Party = GSTown->FindPartyByPlayer(LeaderName);
	if (!Party)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartDungeon] %s is not in a party."), *LeaderName);
		return;
	}

	if (!Party->IsLeader(LeaderName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartDungeon] %s is not the party leader."), *LeaderName);
		return;
	}

	// MVP: 준비 체크 스킵 가능 (주석 해제하면 활성화)
	/*
	if (!Party->IsAllReady())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartDungeon] Not all members are ready."));
		return;
	}
	*/

	const FString TargetDungeon = Party->SelectedDungeon;
	UE_LOG(LogTemp, Log, TEXT("[StartDungeon] PartyId=%d, Leader=%s, Members=%d, Dungeon=%s"),
		Party->PartyId, *LeaderName, Party->Members.Num(), *TargetDungeon);

	// 모든 파티원에게 TravelToMap 호출
	for (const auto& Member : Party->Members)
	{
		// PlayerName으로 현재 접속 중인 PlayerController 찾기
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
				{
					if (PS->GetPlayerName() == Member.PlayerName)
					{
						if (AD1PlayerController* D1PC = Cast<AD1PlayerController>(PC))
						{
							UE_LOG(LogTemp, Log, TEXT("[StartDungeon] Traveling member: %s"), *Member.PlayerName);
							D1PC->TravelToMap(TargetDungeon);
						}
						break;
					}
				}
			}
		}
	}
}

void AD1GameModeTown::ShowLoadingForPartyMembers(const TArray<FString>& MemberNames)
{
	// TODO: Client RPC로 각 멤버에게 로딩 화면 표시 지시
	// 현재는 TravelToMap 자체가 로딩을 유발하므로 별도 구현은 Phase 2에서
}

