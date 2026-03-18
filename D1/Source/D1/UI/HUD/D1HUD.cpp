// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/D1HUD.h"

#include "UI/Widget/D1UserWidget.h"
#include "UI/WidgetController/D1OverlayWidgetController.h"
#include "UI/WidgetController/D1AttributeMenuWidgetController.h"
#include "UI/WidgetController//D1SkillMenuWidgetController.h"
#include "Blueprint/UserWidget.h"

UD1OverlayWidgetController* AD1HUD::GetOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	if (OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UD1OverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		OverlayWidgetController->BindCallbacksToDependencies();
	}
	return OverlayWidgetController;
}

UD1AttributeMenuWidgetController* AD1HUD::GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams)
{
	if (AttributeMenuWidgetController == nullptr)
	{
		AttributeMenuWidgetController = NewObject<UD1AttributeMenuWidgetController>(this, AttributeMenuWidgetControllerClass);
		AttributeMenuWidgetController->SetWidgetControllerParams(WCParams);
		AttributeMenuWidgetController->BindCallbacksToDependencies();
	}
	return AttributeMenuWidgetController;
}

UD1SkillMenuWidgetController* AD1HUD::GetSkillMenuWidgetController(const FWidgetControllerParams& WCParams)
{
	if (SkillMenuWidgetController == nullptr)
	{
		SkillMenuWidgetController = NewObject<UD1SkillMenuWidgetController>(this, SkillMenuWidgetControllerClass);
		SkillMenuWidgetController->SetWidgetControllerParams(WCParams);
		SkillMenuWidgetController->BindCallbacksToDependencies();
	}
	return SkillMenuWidgetController;
}

void AD1HUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	checkf(OverlayWidgetClass, TEXT("Overlay Widget Class uninitialized, please fill out BP_D1HUD"));
	checkf(OverlayWidgetControllerClass, TEXT("Overlay Widget Controller Class uninitialized, please fill out BP_D1HUD"));

	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	OverlayWidget = Cast<UD1UserWidget>(Widget);

	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
	UD1OverlayWidgetController* WidgetController = GetOverlayWidgetController(WidgetControllerParams);

	OverlayWidget->SetWidgetController(WidgetController);
	WidgetController->BroadcastInitialValues();

	// ============================================================
	// 3. WBP_Spells 전용 컨트롤러 주입 (핵심 부분)
	// ============================================================
	// WBP_Overlay(부모) 내부에서 "WBP_Spells"라는 이름의 자식 위젯을 찾습니다.
	if (UD1UserWidget* SpellsWidget = Cast<UD1UserWidget>(OverlayWidget->GetWidgetFromName(TEXT("WBP_Spells"))))
	{
		// 스킬 메뉴용 컨트롤러를 가져와서 꽂아줍니다.
		UD1SkillMenuWidgetController* SkillMenuController = GetSkillMenuWidgetController(WidgetControllerParams);

		SpellsWidget->SetWidgetController(SkillMenuController);

		// 스킬 메뉴의 초기값(스킬 포인트 등)을 방송합니다.
		SkillMenuController->BroadcastInitialValues();

		UE_LOG(LogTemp, Warning, TEXT("WBP_Spells: SkillMenuWidgetController success"));
	}
	else
	{
		// 위젯을 찾지 못했다면 로그를 남겨 확인합니다.
		UE_LOG(LogTemp, Error, TEXT("WBP_Spells can't find"));
	}
	// ============================================================

	Widget->AddToViewport();
}