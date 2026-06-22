// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1PreGameWidgetController.h"
#include "Game/D1HttpSubsystem.h"

void UD1PreGameWidgetController::Init(UD1HttpSubsystem* InHttpSubsystem)
{
	HttpSubsystem = InHttpSubsystem;
	HttpSubsystem->OnLoginResponse.AddDynamic(this, &UD1PreGameWidgetController::HandleLoginResponse);
	HttpSubsystem->OnGetCharactersResponse.AddDynamic(this, &UD1PreGameWidgetController::HandleGetCharactersResponse);
	HttpSubsystem->OnRegisterResponse.AddDynamic(this, &UD1PreGameWidgetController::HandleRegisterResponse);
	HttpSubsystem->OnCreateCharacterResponse.AddDynamic(this, &UD1PreGameWidgetController::HandleCreateCharacterResponse);
	HttpSubsystem->OnEnterTownResponse.AddDynamic(this, &UD1PreGameWidgetController::HandleEnterTownResponse);
}

void UD1PreGameWidgetController::RequestLogin(const FString& Email, const FString& Password)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->Login(Email, Password);
	}
}

void UD1PreGameWidgetController::RequestRegister(const FString& Email, const FString& Password)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->Register(Email, Password);
	}
}

void UD1PreGameWidgetController::RequestGetCharacters()
{
	if (HttpSubsystem)
	{
		HttpSubsystem->GetCharacters();
	}
}

void UD1PreGameWidgetController::RequestCreateCharacter(const FString& Name, const FString& ClassType)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->CreateCharacter(Name, ClassType);
	}
}

void UD1PreGameWidgetController::RequestEnterTown(int64 CharacterId)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->EnterTown(CharacterId);
	}
}

TArray<FD1CharacterInfo> UD1PreGameWidgetController::GetCharacters() const
{
	if (HttpSubsystem)
	{
		return HttpSubsystem->Characters;
	}
	return TArray<FD1CharacterInfo>();
}

void UD1PreGameWidgetController::HandleLoginResponse(bool bSuccess, const FString& ErrorMessage)
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

void UD1PreGameWidgetController::HandleGetCharactersResponse(bool bSuccess, const FString& ErrorMessage)
{
	// 목록 로드 성공/실패 무관하게 캐릭터 선택창으로 진입 (빈 목록이면 생성 유도)
	OnLoginSuccess.Broadcast();

	if (bSuccess)
	{
		OnCharacterListUpdated.Broadcast();
	}
}

void UD1PreGameWidgetController::HandleRegisterResponse(bool bSuccess, const FString& ErrorMessage)
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

void UD1PreGameWidgetController::HandleCreateCharacterResponse(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnCharacterListUpdated.Broadcast();
	}
	else
	{
		OnCharacterCreateFailed.Broadcast(ErrorMessage);
	}
}

void UD1PreGameWidgetController::HandleEnterTownResponse(bool bSuccess, const FString& ErrorMessage)
{
	// 성공 시엔 ClientTravel로 레벨이 바뀌므로 실패만 처리
	if (!bSuccess)
	{
		OnEnterTownFailed.Broadcast(ErrorMessage);
	}
}
