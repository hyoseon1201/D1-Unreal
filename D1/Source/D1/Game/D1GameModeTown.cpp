// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeTown.h"
#include "D1/D1.h"
#include "Game/D1GameStateTown.h"
#include "Game/D1HttpSubsystem.h"
#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

AD1GameModeTown::AD1GameModeTown()
{
	GameStateClass = AD1GameStateTown::StaticClass();
}

void AD1GameModeTown::Logout(AController* Exiting)
{
	// 파티 자동 탈퇴: Super 전에 처리해 PlayerState가 살아있을 때 접근
	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		if (AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>())
		{
			if (AD1GameStateTown* GSTown = GetGameStateTown())
			{
				const FString PartyPlayerId = PS->GetPartyPlayerId();
				if (FD1PartyInfo* Party = GSTown->FindPartyByPlayerId(PartyPlayerId))
				{
					UE_LOG(LogD1Party, Log, TEXT("Logout 파티 자동 탈퇴: %s (PartyId=%d)"),
						*PS->GetPlayerName(), Party->PartyId);
					GSTown->ServerLeaveParty(Party->PartyId, PartyPlayerId);
				}
			}
		}
	}

	Super::Logout(Exiting);
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

	// 파티원 PlayerId 셋 구성 (빠른 멤버십 확인용)
	TSet<FString> PartyMemberIds;
	for (const FD1PartyMemberInfo& Member : Party->Members)
	{
		PartyMemberIds.Add(Member.PlayerId);
	}

	UWorld* World = GetWorld();
	UD1HttpSubsystem* Http = GetGameInstance() ? GetGameInstance()->GetSubsystem<UD1HttpSubsystem>() : nullptr;

	// DB 로그인 여부로 크로스 프로세스 경로 결정 (리더 기준)
	const bool bCrossProcess = Http != nullptr && LeaderPS->WebCharacterId > 0;

	if (bCrossProcess)
	{
		// 크로스 프로세스: 파티원 각자 Save → IssueToken → ClientTravel (개별 비동기)
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			AD1PlayerController* PC = Cast<AD1PlayerController>(It->Get());
			if (!PC) continue;

			AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
			if (!PS || !PartyMemberIds.Contains(PS->GetPartyPlayerId())) continue;

			if (PS->WebCharacterId <= 0)
			{
				UE_LOG(LogD1Party, Warning, TEXT("StartDungeon: %s는 WebCharacterId 없음 — 스킵"), *PS->GetPlayerName());
				continue;
			}

			// fire-and-forget 저장 후 토큰 발급 → ClientTravel
			Http->SaveCharacter(PS->WebCharacterId, PS->BuildSaveJson());

			const int64 CharId = PS->WebCharacterId;
			TWeakObjectPtr<AD1PlayerController> WeakPC(PC);
			FD1IssueSessionDelegate Delegate;
			Delegate.BindLambda([WeakPC, CharId](bool bSuccess, const FString& Token, const FString& Address)
			{
				if (!bSuccess)
				{
					UE_LOG(LogD1Travel, Warning, TEXT("StartDungeon: 세션 토큰 발급 실패 CharId=%lld"), CharId);
					return;
				}
				if (!WeakPC.IsValid())
				{
					UE_LOG(LogD1Travel, Warning, TEXT("StartDungeon: PC가 이미 소멸됨 CharId=%lld"), CharId);
					return;
				}
				const FString URL = FString::Printf(TEXT("%s?sessionToken=%s"), *Address, *Token);
				UE_LOG(LogD1Travel, Log, TEXT("StartDungeon: ClientTravel → %s (CharId=%lld)"), *Address, CharId);
				WeakPC->ClientTravel(URL, ETravelType::TRAVEL_Absolute);
			});
			Http->IssueSessionToken(CharId, TEXT("dungeon"), Delegate);
		}
	}
	else
	{
		// PIE 폴백: GameInstance TMap 저장 후 ServerTravel (같은 프로세스 내 이동)
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AD1PlayerController* PC = Cast<AD1PlayerController>(It->Get()))
			{
				PC->SaveTravelDataToGameInstance();
			}
		}

		FString TravelURL = TargetDungeon;
		if (!TravelURL.StartsWith(TEXT("/")))
		{
			TravelURL = FString::Printf(TEXT("/Game/Maps/%s"), *TargetDungeon);
		}
		UE_LOG(LogD1Party, Log, TEXT("StartDungeon: ServerTravel(PIE) → %s"), *TravelURL);
		World->ServerTravel(TravelURL);
	}
}

void AD1GameModeTown::ShowLoadingForPartyMembers(const TArray<FString>& MemberNames)
{
	// TODO: Client RPC로 각 멤버에게 로딩 화면 표시 지시
	// 현재는 TravelToMap 자체가 로딩을 유발하므로 별도 구현은 Phase 2에서
}

