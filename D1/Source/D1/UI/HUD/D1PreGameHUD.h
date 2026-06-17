// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "D1PreGameHUD.generated.h"

class UD1UserWidget;
class UD1LoginWidgetController;
class UD1CharacterSelectWidgetController;
class UD1HttpSubsystem;

/**
 * 로그인/캐릭터 선택 화면을 관리하는 HUD.
 * GAS 없는 사전 화면 전용이므로 D1HUD와 분리.
 */
UCLASS()
class D1_API AD1PreGameHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	/** 로그인 성공 후 캐릭터 선택 화면으로 전환 */
	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void ShowCharacterSelect();

private:
	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1UserWidget> LoginWidgetClass;

	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1LoginWidgetController> LoginWidgetControllerClass;

	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1UserWidget> CharacterSelectWidgetClass;

	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1CharacterSelectWidgetController> CharacterSelectWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UD1UserWidget> LoginWidget;

	UPROPERTY()
	TObjectPtr<UD1LoginWidgetController> LoginWidgetController;

	UPROPERTY()
	TObjectPtr<UD1UserWidget> CharacterSelectWidget;

	UPROPERTY()
	TObjectPtr<UD1CharacterSelectWidgetController> CharacterSelectWidgetController;
};
