// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/D1GameModeBase.h"
#include "D1GameModeTown.generated.h"

class AD1PlayerController;
class AD1GameStateTown;

/**
 * 마을 전용 게임 모드
 * 파티 생성/관리 및 던전 입장을 처리
 */
UCLASS()
class D1_API AD1GameModeTown : public AD1GameModeBase
{
	GENERATED_BODY()

public:
	AD1GameModeTown();

	/** 마을에서는 전투/버프/회복 등 모든 Ability 사용 불가 */
	virtual bool AreAbilitiesAllowed() const override { return false; }

	/** 파티장이 던전 시작을 요청하면 모든 파티원을 해당 던전으로 이동시킴 */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void StartDungeonForParty(AD1PlayerController* LeaderPC);

	/** 파티원들에게 로딩 화면을 표시하도록 지시 (Client RPC) */
	UFUNCTION(BlueprintCallable, Category = "Party")
	void ShowLoadingForPartyMembers(const TArray<FString>& MemberNames);

	/** 연결 종료 시 파티 자동 탈퇴 (던전 이동·강제 종료 등 모든 경로 처리) */
	virtual void Logout(AController* Exiting) override;

protected:
	/** GameStateTown 캐스팅 헬퍼 */
	AD1GameStateTown* GetGameStateTown() const;
};
