// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "D1GameMenuWidgetController.generated.h"

/**
 * 인게임 게임 메뉴 (ESC 메뉴 등) WidgetController.
 * 타이틀 복귀 / 게임 종료를 처리한다.
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1GameMenuWidgetController : public UD1WidgetController
{
	GENERATED_BODY()

public:
	/** 타이틀로 복귀. 레벨 전환 시 서버 Logout이 자동 호출되어 캐릭터가 저장된다. */
	UFUNCTION(BlueprintCallable, Category = "D1|GameMenu")
	void RequestReturnToTitle();

	/** 게임 완전 종료. */
	UFUNCTION(BlueprintCallable, Category = "D1|GameMenu")
	void RequestQuitGame();

	/** 복귀할 프리게임 맵 이름 (BP_D1GameMenuWidgetController에서 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "D1|GameMenu")
	FName PreGameMapName = FName("PreGame");
};
