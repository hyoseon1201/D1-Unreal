// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/D1CharacterBase.h"
#include "D1Hero.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

/**
 * 
 */
UCLASS()
class D1_API AD1Hero : public AD1CharacterBase
{
	GENERATED_BODY()

public:
	AD1Hero();

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	// combat interface
	virtual int32 GetPlayerLevel_Implementation() override;

private:
	virtual void InitAbilityActorInfo() override;
};
