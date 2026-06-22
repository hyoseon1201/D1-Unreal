// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/D1PreGameHUD.h"
#include "UI/Widget/D1UserWidget.h"
#include "UI/WidgetController/D1PreGameWidgetController.h"
#include "Game/D1HttpSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "D1.h"

void AD1PreGameHUD::BeginPlay()
{
	Super::BeginPlay();

	// 메뉴 화면이므로 마우스 커서 표시 + UI 전용 입력 모드 (클릭 시 커서 캡처/숨김 방지)
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	UD1HttpSubsystem* HttpSubsystem = GetGameInstance()->GetSubsystem<UD1HttpSubsystem>();
	if (!HttpSubsystem)
	{
		UE_LOG(LogD1, Error, TEXT("D1PreGameHUD: HttpSubsystem을 찾을 수 없습니다."));
		return;
	}

	// 프리게임 WidgetController 생성 및 초기화 (로그인~Town 입장까지 단일 컨트롤러 공유)
	if (PreGameWidgetControllerClass)
	{
		PreGameWidgetController = NewObject<UD1PreGameWidgetController>(this, PreGameWidgetControllerClass);
		PreGameWidgetController->Init(HttpSubsystem);
	}

	// 프리게임 오버레이 위젯 생성. Login/Register/CharacterSelect/NewCharacter 패널 전환은 BP 내부에서 처리
	if (PreGameOverlayWidgetClass)
	{
		PreGameOverlayWidget = CreateWidget<UD1UserWidget>(GetOwningPlayerController(), PreGameOverlayWidgetClass);
		if (PreGameOverlayWidget)
		{
			PreGameOverlayWidget->SetWidgetController(PreGameWidgetController);
			PreGameOverlayWidget->AddToViewport();
		}
	}
}
