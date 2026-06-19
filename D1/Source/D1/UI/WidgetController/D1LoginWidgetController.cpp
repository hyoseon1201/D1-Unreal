// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1LoginWidgetController.h"
#include "Game/D1HttpSubsystem.h"

void UD1LoginWidgetController::Init(UD1HttpSubsystem* InHttpSubsystem)
{
	HttpSubsystem = InHttpSubsystem;
	HttpSubsystem->OnLoginResponse.AddDynamic(this, &UD1LoginWidgetController::HandleLoginResponse);
	HttpSubsystem->OnGetCharactersResponse.AddDynamic(this, &UD1LoginWidgetController::HandleGetCharactersResponse);
	HttpSubsystem->OnRegisterResponse.AddDynamic(this, &UD1LoginWidgetController::HandleRegisterResponse);
}

void UD1LoginWidgetController::RequestLogin(const FString& Email, const FString& Password)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->Login(Email, Password);
	}
}

void UD1LoginWidgetController::RequestRegister(const FString& Email, const FString& Password)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->Register(Email, Password);
	}
}

void UD1LoginWidgetController::HandleLoginResponse(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		// 캐릭터 목록 먼저 받은 후 OnLoginSuccess 브로드캐스트
		HttpSubsystem->GetCharacters();
	}
	else
	{
		OnLoginFailed.Broadcast(ErrorMessage);
	}
}

void UD1LoginWidgetController::HandleGetCharactersResponse(bool bSuccess, const FString& ErrorMessage)
{
	// 목록 로드 성공/실패 무관하게 캐릭터 선택창으로 진입 (빈 목록이면 생성 유도)
	OnLoginSuccess.Broadcast();
}

void UD1LoginWidgetController::HandleRegisterResponse(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnRegisterSuccess.Broadcast();
	}
	else
	{
		OnRegisterFailed.Broadcast(ErrorMessage);
	}
}
