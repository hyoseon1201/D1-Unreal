// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1GameMenuWidgetController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UD1GameMenuWidgetController::RequestReturnToTitle()
{
	// 레벨 전환 → 서버와 연결 해제 → GameModeBase::Logout 호출 → 캐릭터 자동 저장
	UGameplayStatics::OpenLevel(this, PreGameMapName);
}

void UD1GameMenuWidgetController::RequestQuitGame()
{
	if (PlayerController)
	{
		UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
	}
}
