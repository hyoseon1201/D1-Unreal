// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1CharacterSelectWidgetController.h"
#include "Game/D1HttpSubsystem.h"

void UD1CharacterSelectWidgetController::Init(UD1HttpSubsystem* InHttpSubsystem)
{
	HttpSubsystem = InHttpSubsystem;
	HttpSubsystem->OnGetCharactersResponse.AddDynamic(this, &UD1CharacterSelectWidgetController::HandleGetCharactersResponse);
	HttpSubsystem->OnCreateCharacterResponse.AddDynamic(this, &UD1CharacterSelectWidgetController::HandleCreateCharacterResponse);
	HttpSubsystem->OnEnterTownResponse.AddDynamic(this, &UD1CharacterSelectWidgetController::HandleEnterTownResponse);
}

void UD1CharacterSelectWidgetController::RequestGetCharacters()
{
	if (HttpSubsystem)
	{
		HttpSubsystem->GetCharacters();
	}
}

void UD1CharacterSelectWidgetController::RequestCreateCharacter(const FString& Name, const FString& ClassType)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->CreateCharacter(Name, ClassType);
	}
}

void UD1CharacterSelectWidgetController::RequestEnterTown(int64 CharacterId)
{
	if (HttpSubsystem)
	{
		HttpSubsystem->EnterTown(CharacterId);
	}
}

TArray<FD1CharacterInfo> UD1CharacterSelectWidgetController::GetCharacters() const
{
	if (HttpSubsystem)
	{
		return HttpSubsystem->Characters;
	}
	return TArray<FD1CharacterInfo>();
}

void UD1CharacterSelectWidgetController::HandleGetCharactersResponse(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnCharacterListUpdated.Broadcast();
	}
}

void UD1CharacterSelectWidgetController::HandleCreateCharacterResponse(bool bSuccess, const FString& ErrorMessage)
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

void UD1CharacterSelectWidgetController::HandleEnterTownResponse(bool bSuccess, const FString& ErrorMessage)
{
	// 성공 시엔 ClientTravel로 레벨이 바뀌므로 실패만 처리
	if (!bSuccess)
	{
		OnEnterTownFailed.Broadcast(ErrorMessage);
	}
}
