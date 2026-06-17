// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "D1GameModeBase.generated.h"

class UD1CharacterClassInfo;
class UD1AbilitySystemConfig;
class UD1AbilityInfo;
class UD1ItemRegistry;
class AD1GameStateBase;
class AD1PlayerState;

/**
 * 
 */
UCLASS()
class D1_API AD1GameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AD1GameModeBase();

	virtual void PostInitializeComponents() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 접속 URL 옵션(?sessionToken=)을 파싱해 PlayerState에 보관 */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal = TEXT("")) override;

	/** 이 GameMode에서 Ability 사용(전투/버프/회복 등)을 허용하는가? */
	UFUNCTION(BlueprintPure, Category = "D1|GameMode Rules")
	virtual bool AreAbilitiesAllowed() const { return true; }

	UPROPERTY(EditDefaultsOnly, Category = "Characer Class Defaults")
	TObjectPtr<UD1CharacterClassInfo> CharacterClassInfo;

	UPROPERTY(EditDefaultsOnly, Category = "Characer Class Defaults")
	TObjectPtr<UD1AbilitySystemConfig> AbilitySystemConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Info")
	TObjectPtr<UD1AbilityInfo> AbilityInfo;

	UPROPERTY(EditDefaultsOnly, Category = "Item Data")
	TObjectPtr<UD1ItemRegistry> ItemRegistry;
};
