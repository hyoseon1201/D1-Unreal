// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1DungeonPartyWidgetController.h"
#include "Game/D1GameStateTown.h"
#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"

// ─── 초기화 ──────────────────────────────────────────────────────────────────

void UD1DungeonPartyWidgetController::BindCallbacksToDependencies()
{
	// GameStateTown의 파티 목록 변경 델리게이트에 바인딩
	if (AD1GameStateTown* GSTown = GetGameStateTown())
	{
		GSTown->OnPartiesChangedDelegate.AddDynamic(this, &UD1DungeonPartyWidgetController::HandlePartiesChanged);
	}
}

void UD1DungeonPartyWidgetController::BroadcastInitialValues()
{
	// 위젯이 처음 열릴 때 현재 파티 목록을 즉시 전달
	if (const AD1GameStateTown* GSTown = GetGameStateTown())
	{
		OnPartiesUpdated.Broadcast(GSTown->Parties);
	}
}

// ─── 파티 액션 ───────────────────────────────────────────────────────────────

void UD1DungeonPartyWidgetController::CreateParty(const FString& DungeonMap, const FString& PartyName)
{
	if (AD1PlayerController* PC = GetD1PC())
	{
		PC->Server_CreateParty(DungeonMap, PartyName);
	}
}

void UD1DungeonPartyWidgetController::JoinParty(int32 PartyId)
{
	if (AD1PlayerController* PC = GetD1PC())
	{
		PC->Server_JoinParty(PartyId);
	}
}

void UD1DungeonPartyWidgetController::LeaveParty()
{
	if (AD1PlayerController* PC = GetD1PC())
	{
		PC->Server_LeaveParty();
	}
}

void UD1DungeonPartyWidgetController::SetReady(bool bReady)
{
	if (AD1PlayerController* PC = GetD1PC())
	{
		PC->Server_SetReady(bReady);
	}
}

void UD1DungeonPartyWidgetController::SelectDungeon(const FString& DungeonMap)
{
	if (AD1PlayerController* PC = GetD1PC())
	{
		PC->Server_SetSelectedDungeon(DungeonMap);
	}
}

void UD1DungeonPartyWidgetController::StartDungeon()
{
	if (AD1PlayerController* PC = GetD1PC())
	{
		PC->Server_StartDungeon();
	}
}

// ─── 쿼리 ────────────────────────────────────────────────────────────────────

bool UD1DungeonPartyWidgetController::IsInParty() const
{
	if (const AD1GameStateTown* GSTown = GetGameStateTown())
	{
		return GSTown->FindPartyByPlayerId(GetLocalPlayerId()) != nullptr;
	}
	return false;
}

bool UD1DungeonPartyWidgetController::IsPartyLeader() const
{
	if (const AD1GameStateTown* GSTown = GetGameStateTown())
	{
		const FString MyId = GetLocalPlayerId();
		if (const FD1PartyInfo* Party = GSTown->FindPartyByPlayerId(MyId))
		{
			return Party->IsLeader(MyId);
		}
	}
	return false;
}

FD1PartyInfo UD1DungeonPartyWidgetController::GetMyParty() const
{
	if (const AD1GameStateTown* GSTown = GetGameStateTown())
	{
		if (const FD1PartyInfo* Party = GSTown->FindPartyByPlayerId(GetLocalPlayerId()))
		{
			return *Party;
		}
	}
	// 파티에 속하지 않으면 PartyId == INDEX_NONE 인 빈 구조체 반환
	return FD1PartyInfo();
}

// ─── Private 헬퍼 ────────────────────────────────────────────────────────────

void UD1DungeonPartyWidgetController::HandlePartiesChanged()
{
	if (const AD1GameStateTown* GSTown = GetGameStateTown())
	{
		OnPartiesUpdated.Broadcast(GSTown->Parties);
	}
}

AD1GameStateTown* UD1DungeonPartyWidgetController::GetGameStateTown() const
{
	if (PlayerController)
	{
		return Cast<AD1GameStateTown>(PlayerController->GetWorld()->GetGameState());
	}
	return nullptr;
}

FString UD1DungeonPartyWidgetController::GetLocalPlayerId() const
{
	if (PlayerController)
	{
		if (const AD1PlayerState* PS = PlayerController->GetPlayerState<AD1PlayerState>())
		{
			return PS->GetPartyPlayerId();
		}
	}
	return FString();
}
