// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Game/D1HttpSubsystem.h"
#include "D1CharacterSelectWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FD1OnCharacterListUpdatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1OnCharacterCreateFailedDelegate, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1OnEnterTownFailedDelegate, const FString&, ErrorMessage);

/**
 * 캐릭터 선택/생성 화면 WidgetController.
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1CharacterSelectWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void Init(UD1HttpSubsystem* InHttpSubsystem);

	/** 캐릭터 목록 요청 */
	UFUNCTION(BlueprintCallable, Category = "D1|CharacterSelect")
	void RequestGetCharacters();

	/** 캐릭터 생성 요청 */
	UFUNCTION(BlueprintCallable, Category = "D1|CharacterSelect")
	void RequestCreateCharacter(const FString& Name, const FString& ClassType);

	/** 캐릭터 선택 후 Town 입장 (매치메이킹 → 세션 토큰 → ClientTravel) */
	UFUNCTION(BlueprintCallable, Category = "D1|CharacterSelect")
	void RequestEnterTown(int64 CharacterId);

	/** 현재 캐릭터 목록 (HttpSubsystem에서 참조) */
	UFUNCTION(BlueprintPure, Category = "D1|CharacterSelect")
	TArray<FD1CharacterInfo> GetCharacters() const;

	UPROPERTY(BlueprintAssignable, Category = "D1|CharacterSelect")
	FD1OnCharacterListUpdatedDelegate OnCharacterListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "D1|CharacterSelect")
	FD1OnCharacterCreateFailedDelegate OnCharacterCreateFailed;

	UPROPERTY(BlueprintAssignable, Category = "D1|CharacterSelect")
	FD1OnEnterTownFailedDelegate OnEnterTownFailed;

private:
	UPROPERTY()
	TObjectPtr<UD1HttpSubsystem> HttpSubsystem;

	UFUNCTION()
	void HandleGetCharactersResponse(bool bSuccess, const FString& ErrorMessage);

	UFUNCTION()
	void HandleCreateCharacterResponse(bool bSuccess, const FString& ErrorMessage);

	UFUNCTION()
	void HandleEnterTownResponse(bool bSuccess, const FString& ErrorMessage);
};
