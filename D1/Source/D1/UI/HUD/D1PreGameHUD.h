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

	/** 로그인 화면에서 회원가입 화면으로 전환 */
	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void ShowRegister();

	/** 회원가입 완료/취소 후 로그인 화면으로 복귀 */
	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void ShowLogin();

private:
	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1UserWidget> LoginWidgetClass;

	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1LoginWidgetController> LoginWidgetControllerClass;

	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1UserWidget> CharacterSelectWidgetClass;

	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1CharacterSelectWidgetController> CharacterSelectWidgetControllerClass;

	/** 회원가입 위젯도 로그인과 동일한 LoginWidgetController를 공유 (RequestRegister가 그 안에 있음) */
	UPROPERTY(EditAnywhere, Category = "D1|PreGame")
	TSubclassOf<UD1UserWidget> RegisterWidgetClass;

	UPROPERTY()
	TObjectPtr<UD1UserWidget> LoginWidget;

	UPROPERTY()
	TObjectPtr<UD1LoginWidgetController> LoginWidgetController;

	UPROPERTY()
	TObjectPtr<UD1UserWidget> CharacterSelectWidget;

	UPROPERTY()
	TObjectPtr<UD1CharacterSelectWidgetController> CharacterSelectWidgetController;

	UPROPERTY()
	TObjectPtr<UD1UserWidget> RegisterWidget;
};
