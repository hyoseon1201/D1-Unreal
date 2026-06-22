// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Game/D1HttpSubsystem.h"
#include "D1PreGameWidgetController.generated.h"

class UD1HttpSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FD1OnLoginSuccessDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1OnLoginFailedDelegate, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FD1OnRegisterSuccessDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FD1OnCharacterListUpdatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1OnCharacterCreateFailedDelegate, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1OnEnterTownFailedDelegate, const FString&, ErrorMessage);

/**
 * 프리게임 화면(로그인/회원가입/캐릭터 선택/생성) WidgetController.
 * 로그인부터 Town 입장까지가 하나의 선형 플로우이므로 단일 컨트롤러로 통합.
 * GAS 없는 사전 화면이므로 UD1WidgetController를 상속하지 않는다.
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1PreGameWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void Init(UD1HttpSubsystem* InHttpSubsystem);

	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void RequestLogin(const FString& Email, const FString& Password);

	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void RequestRegister(const FString& Email, const FString& Password);

	/** 캐릭터 목록 요청 */
	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void RequestGetCharacters();

	/** 캐릭터 생성 요청 */
	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void RequestCreateCharacter(const FString& Name, const FString& ClassType);

	/** 캐릭터 선택 후 Town 입장 (매치메이킹 → 세션 토큰 → ClientTravel) */
	UFUNCTION(BlueprintCallable, Category = "D1|PreGame")
	void RequestEnterTown(int64 CharacterId);

	/** 현재 캐릭터 목록 (HttpSubsystem에서 참조) */
	UFUNCTION(BlueprintPure, Category = "D1|PreGame")
	TArray<FD1CharacterInfo> GetCharacters() const;

	UPROPERTY(BlueprintAssignable, Category = "D1|PreGame")
	FD1OnLoginSuccessDelegate OnLoginSuccess;

	UPROPERTY(BlueprintAssignable, Category = "D1|PreGame")
	FD1OnLoginFailedDelegate OnLoginFailed;

	UPROPERTY(BlueprintAssignable, Category = "D1|PreGame")
	FD1OnRegisterSuccessDelegate OnRegisterSuccess;

	UPROPERTY(BlueprintAssignable, Category = "D1|PreGame")
	FD1OnLoginFailedDelegate OnRegisterFailed;

	UPROPERTY(BlueprintAssignable, Category = "D1|PreGame")
	FD1OnCharacterListUpdatedDelegate OnCharacterListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "D1|PreGame")
	FD1OnCharacterCreateFailedDelegate OnCharacterCreateFailed;

	UPROPERTY(BlueprintAssignable, Category = "D1|PreGame")
	FD1OnEnterTownFailedDelegate OnEnterTownFailed;

private:
	UPROPERTY()
	TObjectPtr<UD1HttpSubsystem> HttpSubsystem;

	UFUNCTION()
	void HandleLoginResponse(bool bSuccess, const FString& ErrorMessage);

	UFUNCTION()
	void HandleGetCharactersResponse(bool bSuccess, const FString& ErrorMessage);

	UFUNCTION()
	void HandleRegisterResponse(bool bSuccess, const FString& ErrorMessage);

	UFUNCTION()
	void HandleCreateCharacterResponse(bool bSuccess, const FString& ErrorMessage);

	UFUNCTION()
	void HandleEnterTownResponse(bool bSuccess, const FString& ErrorMessage);
};
