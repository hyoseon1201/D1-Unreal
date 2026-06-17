// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "D1LoginWidgetController.generated.h"

class UD1HttpSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FD1OnLoginSuccessDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1OnLoginFailedDelegate, const FString&, ErrorMessage);

/**
 * 로그인 화면 WidgetController.
 * GAS 없는 사전 화면이므로 UD1WidgetController를 상속하지 않는다.
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1LoginWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void Init(UD1HttpSubsystem* InHttpSubsystem);

	UFUNCTION(BlueprintCallable, Category = "D1|Login")
	void RequestLogin(const FString& Email, const FString& Password);

	UPROPERTY(BlueprintAssignable, Category = "D1|Login")
	FD1OnLoginSuccessDelegate OnLoginSuccess;

	UPROPERTY(BlueprintAssignable, Category = "D1|Login")
	FD1OnLoginFailedDelegate OnLoginFailed;

private:
	UPROPERTY()
	TObjectPtr<UD1HttpSubsystem> HttpSubsystem;

	UFUNCTION()
	void HandleLoginResponse(bool bSuccess, const FString& ErrorMessage);

	UFUNCTION()
	void HandleGetCharactersResponse(bool bSuccess, const FString& ErrorMessage);
};
