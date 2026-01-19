// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "D1GameModeBase.generated.h"

class UD1CharacterClassInfo;

/**
 * 
 */
UCLASS()
class D1_API AD1GameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, Category = "Characer Class Defaults")
	TObjectPtr<UD1CharacterClassInfo> CharacterClassInfo;
};
