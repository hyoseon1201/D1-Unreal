// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "D1PreGameHUD.generated.h"

class UD1UserWidget;
class UD1PreGameWidgetController;
class UD1HttpSubsystem;

/**
 * 로그인/회원가입/캐릭터 선택/생성 화면을 관리하는 HUD.
 * 모든 화면은 WBP_PreGameOverlay 하나(서브패널 전환은 BP 내부에서 처리)로 묶여있다.
 * GAS 없는 사전 화면 전용이므로 D1HUD와 분리.
 */
UCLASS()
class D1_API AD1PreGameHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1UserWidget> PreGameOverlayWidgetClass;

	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1PreGameWidgetController> PreGameWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UD1UserWidget> PreGameOverlayWidget;

	UPROPERTY()
	TObjectPtr<UD1PreGameWidgetController> PreGameWidgetController;
};
