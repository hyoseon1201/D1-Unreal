// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameStateTown.h"
#include "Net/UnrealNetwork.h"

AD1GameStateTown::AD1GameStateTown()
{
}

void AD1GameStateTown::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AD1GameStateTown, Parties);
}

void AD1GameStateTown::OnRep_Parties()
{
	UE_LOG(LogTemp, Log, TEXT("[Party] OnRep_Parties called. Count=%d"), Parties.Num());
	OnPartiesChangedDelegate.Broadcast();
}

bool AD1GameStateTown::ServerCreateParty(const FString& LeaderName, int32 LeaderLevel, const FString& DungeonMap, const FString& PartyName)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	// 이미 다른 파티에 속해있는지 확인
	if (FindPartyByPlayer(LeaderName) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Party] %s is already in a party."), *LeaderName);
		return false;
	}

	// 방 제목이 비어있으면 기본값으로 채워줌
	const FString FinalPartyName = PartyName.IsEmpty() ? FString::Printf(TEXT("%s의 파티"), *LeaderName) : PartyName;

	FD1PartyInfo NewParty;
	NewParty.PartyId = GeneratePartyId();
	NewParty.LeaderName = LeaderName;
	NewParty.PartyName = FinalPartyName;
	NewParty.SelectedDungeon = DungeonMap;

	FD1PartyMemberInfo Leader;
	Leader.PlayerName = LeaderName;
	Leader.PlayerLevel = LeaderLevel;
	Leader.bIsReady = true; // 파티장은 기본적으로 준비 완료
	NewParty.Members.Add(Leader);

	Parties.Add(NewParty);
	OnRep_Parties(); // 서버에서도 강제 브로드캐스트 (OnRep은 클라이언트에서만 자동)

	UE_LOG(LogTemp, Log, TEXT("[Party] Created PartyId=%d, Leader=%s"), NewParty.PartyId, *LeaderName);
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
			UE_LOG(LogTemp, Log, TEXT("[Party] Disbanded PartyId=%d"), PartyId);
			Parties.RemoveAt(i);
			OnRep_Parties();
			return true;
		}
	}
	return false;
}

bool AD1GameStateTown::ServerJoinParty(int32 PartyId, const FString& PlayerName, int32 PlayerLevel)
{
	if (HasAuthority() == false)
	{
		return false;
	}

	// 이미 다른 파티에 속해있는지 확인
	if (FindPartyByPlayer(PlayerName) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Party] %s is already in a party."), *PlayerName);
		return false;
	}

	for (auto& Party : Parties)
	{
		if (Party.PartyId == PartyId)
		{
			if (Party.Members.Num() >= Party.MaxMembers) // 파티별 최대 인원 검증 (데이터 기준, UI 표시값과 항상 일치)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Party] PartyId=%d is full."), PartyId);
				return false;
			}

			FD1PartyMemberInfo NewMember;
			NewMember.PlayerName = PlayerName;
			NewMember.PlayerLevel = PlayerLevel;
			NewMember.bIsReady = false;
			Party.Members.Add(NewMember);

			OnRep_Parties();
			UE_LOG(LogTemp, Log, TEXT("[Party] %s joined PartyId=%d"), *PlayerName, PartyId);
			return true;
		}
	}
	return false;
}

bool AD1GameStateTown::ServerLeaveParty(int32 PartyId, const FString& PlayerName)
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
			if (Party.IsLeader(PlayerName))
			{
				return ServerDisbandParty(PartyId);
			}

			// 멤버 제거
			for (int32 j = 0; j < Party.Members.Num(); ++j)
			{
				if (Party.Members[j].PlayerName == PlayerName)
				{
					Party.Members.RemoveAt(j);
					OnRep_Parties();
					UE_LOG(LogTemp, Log, TEXT("[Party] %s left PartyId=%d"), *PlayerName, PartyId);
					return true;
				}
			}
		}
	}
	return false;
}

bool AD1GameStateTown::ServerSetReady(int32 PartyId, const FString& PlayerName, bool bReady)
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
				if (Member.PlayerName == PlayerName)
				{
					Member.bIsReady = bReady;
					OnRep_Parties();
					UE_LOG(LogTemp, Log, TEXT("[Party] %s ready=%s in PartyId=%d"),
						*PlayerName, bReady ? TEXT("TRUE") : TEXT("FALSE"), PartyId);
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

	for (auto& Party : Parties)
	{
		if (Party.PartyId == PartyId)
		{
			Party.SelectedDungeon = DungeonMap;
			OnRep_Parties();
			UE_LOG(LogTemp, Log, TEXT("[Party] PartyId=%d selected dungeon=%s"), PartyId, *DungeonMap);
			return true;
		}
	}
	return false;
}

FD1PartyInfo* AD1GameStateTown::FindPartyByPlayer(const FString& PlayerName)
{
	for (auto& Party : Parties)
	{
		if (Party.HasMember(PlayerName))
		{
			return &Party;
		}
	}
	return nullptr;
}

const FD1PartyInfo* AD1GameStateTown::FindPartyByPlayer(const FString& PlayerName) const
{
	for (const auto& Party : Parties)
	{
		if (Party.HasMember(PlayerName))
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
