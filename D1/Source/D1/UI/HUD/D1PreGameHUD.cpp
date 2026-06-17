// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/D1PreGameHUD.h"
#include "UI/Widget/D1UserWidget.h"
#include "UI/WidgetController/D1LoginWidgetController.h"
#include "UI/WidgetController/D1CharacterSelectWidgetController.h"
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

	// 로그인 WidgetController 생성 및 초기화
	if (LoginWidgetControllerClass)
	{
		LoginWidgetController = NewObject<UD1LoginWidgetController>(this, LoginWidgetControllerClass);
		LoginWidgetController->Init(HttpSubsystem);
	}

	// 캐릭터 선택 WidgetController 생성 및 초기화
	if (CharacterSelectWidgetControllerClass)
	{
		CharacterSelectWidgetController = NewObject<UD1CharacterSelectWidgetController>(this, CharacterSelectWidgetControllerClass);
		CharacterSelectWidgetController->Init(HttpSubsystem);
	}

	// 로그인 위젯 생성 및 표시
	if (LoginWidgetClass)
	{
		LoginWidget = CreateWidget<UD1UserWidget>(GetOwningPlayerController(), LoginWidgetClass);
		if (LoginWidget)
		{
			LoginWidget->SetWidgetController(LoginWidgetController);
			LoginWidget->AddToViewport();
		}
	}

	// 캐릭터 선택 위젯은 숨긴 채로 생성
	if (CharacterSelectWidgetClass)
	{
		CharacterSelectWidget = CreateWidget<UD1UserWidget>(GetOwningPlayerController(), CharacterSelectWidgetClass);
		if (CharacterSelectWidget)
		{
			CharacterSelectWidget->SetWidgetController(CharacterSelectWidgetController);
			CharacterSelectWidget->AddToViewport();
			CharacterSelectWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void AD1PreGameHUD::ShowCharacterSelect()
{
	if (LoginWidget)
	{
		LoginWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CharacterSelectWidget)
	{
		CharacterSelectWidget->SetVisibility(ESlateVisibility::Visible);
	}
}
