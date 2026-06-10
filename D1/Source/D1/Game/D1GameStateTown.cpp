// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameStateTown.h"
#include "D1/D1.h"
#include "Net/UnrealNetwork.h"

AD1GameStateTown::AD1GameStateTown()
{
	// 입장 가능한 던전 맵 기본 목록 (BP 서브클래스에서 추가/수정 가능)
	AllowedDungeonMaps.Add(TEXT("Dungeon"));
}

bool AD1GameStateTown::IsAllowedDungeonMap(const FString& DungeonMap) const
{
	return AllowedDungeonMaps.Contains(DungeonMap);
}

void AD1GameStateTown::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AD1GameStateTown, Parties);
}

void AD1GameStateTown::OnRep_Parties()
{
	UE_LOG(LogD1Party, Verbose, TEXT("OnRep_Parties called. Count=%d"), Parties.Num());
	OnPartiesChangedDelegate.Broadcast();
}

bool AD1GameStateTown::ServerCreateParty(const FString& LeaderId, const FString& LeaderName, int32 LeaderLevel, const FString& DungeonMap, const FString& PartyName)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	// 이미 다른 파티에 속해있는지 확인
	if (FindPartyByPlayerId(LeaderId) != nullptr)
	{
		UE_LOG(LogD1Party, Warning, TEXT("%s is already in a party."), *LeaderName);
		return false;
	}

	// 클라이언트가 보낸 맵 이름 검증 (조작된 클라이언트의 임의 맵 입장 차단)
	if (!IsAllowedDungeonMap(DungeonMap))
	{
		UE_LOG(LogD1Party, Warning, TEXT("CreateParty rejected: '%s' is not an allowed dungeon map. (Leader=%s)"), *DungeonMap, *LeaderName);
		return false;
	}

	// 방 제목이 비어있으면 기본값으로 채워줌, 최대 길이 초과분은 잘라냄
	const FString FinalPartyName = PartyName.IsEmpty()
		? FString::Printf(TEXT("%s의 파티"), *LeaderName)
		: PartyName.Left(MaxPartyNameLength);

	FD1PartyInfo NewParty;
	NewParty.PartyId = GeneratePartyId();
	NewParty.LeaderId = LeaderId;
	NewParty.LeaderName = LeaderName;
	NewParty.PartyName = FinalPartyName;
	NewParty.SelectedDungeon = DungeonMap;

	FD1PartyMemberInfo Leader;
	Leader.PlayerId = LeaderId;
	Leader.PlayerName = LeaderName;
	Leader.PlayerLevel = LeaderLevel;
	Leader.bIsReady = true; // 파티장은 기본적으로 준비 완료
	NewParty.Members.Add(Leader);

	Parties.Add(NewParty);
	OnRep_Parties(); // 서버에서도 강제 브로드캐스트 (OnRep은 클라이언트에서만 자동)

	UE_LOG(LogD1Party, Log, TEXT("Created PartyId=%d, Leader=%s"), NewParty.PartyId, *LeaderName);
	return true;
}

bool AD1GameStateTown::ServerDisbandParty(int32 PartyId)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	for (int32 i = 0; i < Parties.Num(); ++i)
	{
		if (Parties[i].PartyId == PartyId)
		{
			UE_LOG(LogD1Party, Log, TEXT("Disbanded PartyId=%d"), PartyId);
			Parties.RemoveAt(i);
			OnRep_Parties();
			return true;
		}
	}
	return false;
}

bool AD1GameStateTown::ServerJoinParty(int32 PartyId, const FString& PlayerId, const FString& PlayerName, int32 PlayerLevel)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	// 이미 다른 파티에 속해있는지 확인
	if (FindPartyByPlayerId(PlayerId) != nullptr)
	{
		UE_LOG(LogD1Party, Warning, TEXT("%s is already in a party."), *PlayerName);
		return false;
	}

	for (auto& Party : Parties)
	{
		if (Party.PartyId == PartyId)
		{
			if (Party.Members.Num() >= Party.MaxMembers) // 파티별 최대 인원 검증 (데이터 기준, UI 표시값과 항상 일치)
			{
				UE_LOG(LogD1Party, Warning, TEXT("PartyId=%d is full."), PartyId);
				return false;
			}

			FD1PartyMemberInfo NewMember;
			NewMember.PlayerId = PlayerId;
			NewMember.PlayerName = PlayerName;
			NewMember.PlayerLevel = PlayerLevel;
			NewMember.bIsReady = false;
			Party.Members.Add(NewMember);

			OnRep_Parties();
			UE_LOG(LogD1Party, Log, TEXT("%s joined PartyId=%d"), *PlayerName, PartyId);
			return true;
		}
	}
	return false;
}

bool AD1GameStateTown::ServerLeaveParty(int32 PartyId, const FString& PlayerId)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	for (int32 i = 0; i < Parties.Num(); ++i)
	{
		if (Parties[i].PartyId == PartyId)
		{
			auto& Party = Parties[i];

			// 파티장이 나가면 파티 해산
			if (Party.IsLeader(PlayerId))
			{
				return ServerDisbandParty(PartyId);
			}

			// 멤버 제거
			for (int32 j = 0; j < Party.Members.Num(); ++j)
			{
				if (Party.Members[j].PlayerId == PlayerId)
				{
					UE_LOG(LogD1Party, Log, TEXT("%s left PartyId=%d"), *Party.Members[j].PlayerName, PartyId);
					Party.Members.RemoveAt(j);
					OnRep_Parties();
					return true;
				}
			}
		}
	}
	return false;
}

bool AD1GameStateTown::ServerSetReady(int32 PartyId, const FString& PlayerId, bool bReady)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	for (auto& Party : Parties)
	{
		if (Party.PartyId == PartyId)
		{
			for (auto& Member : Party.Members)
			{
				if (Member.PlayerId == PlayerId)
				{
					Member.bIsReady = bReady;
					OnRep_Parties();
					UE_LOG(LogD1Party, Log, TEXT("%s ready=%s in PartyId=%d"),
						*Member.PlayerName, bReady ? TEXT("TRUE") : TEXT("FALSE"), PartyId);
					return true;
				}
			}
		}
	}
	return false;
}

bool AD1GameStateTown::ServerSelectDungeon(int32 PartyId, const FString& DungeonMap)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	// 클라이언트가 보낸 맵 이름 검증
	if (!IsAllowedDungeonMap(DungeonMap))
	{
		UE_LOG(LogD1Party, Warning, TEXT("SelectDungeon rejected: '%s' is not an allowed dungeon map. (PartyId=%d)"), *DungeonMap, PartyId);
		return false;
	}

	for (auto& Party : Parties)
	{
		if (Party.PartyId == PartyId)
		{
			Party.SelectedDungeon = DungeonMap;
			OnRep_Parties();
			UE_LOG(LogD1Party, Log, TEXT("PartyId=%d selected dungeon=%s"), PartyId, *DungeonMap);
			return true;
		}
	}
	return false;
}

FD1PartyInfo* AD1GameStateTown::FindPartyByPlayerId(const FString& PlayerId)
{
	for (auto& Party : Parties)
	{
		if (Party.HasMember(PlayerId))
		{
			return &Party;
		}
	}
	return nullptr;
}

const FD1PartyInfo* AD1GameStateTown::FindPartyByPlayerId(const FString& PlayerId) const
{
	for (const auto& Party : Parties)
	{
		if (Party.HasMember(PlayerId))
		{
			return &Party;
		}
	}
	return nullptr;
}

int32 AD1GameStateTown::GeneratePartyId()
{
	return NextPartyId++;
}
