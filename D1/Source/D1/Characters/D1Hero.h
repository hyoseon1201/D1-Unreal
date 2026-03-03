// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/D1CharacterBase.h"
#include "Interaction/PlayerInterface.h"
#include "D1Hero.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

/**
 * 
 */
UCLASS()
class D1_API AD1Hero : public AD1CharacterBase, public IPlayerInterface
{
	GENERATED_BODY()

public:
	AD1Hero();

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	// Begin combat interface
	virtual int32 GetPlayerLevel_Implementation() override;
	// End combat interface

	// Begin player interface
	virtual void AddToXP_Implementation(int32 InXP) override;
	virtual int32 GetXP_Implementation() const override;
	virtual void LevelUp_Implementation() override;
	virtual int32 FindLevelForXP_Implementation(int32 InXP) const override;
	virtual int32 GetAttributePointsReward_Implementation(int32 Level) const override;
	virtual int32 GetSpellPointsReward_Implementation(int32 Level) const override;
	virtual void AddToPlayerLevel_Implementation(int32 InPlayerLevel) override;
	virtual void AddToSpellPoints_Implementation(int32 InSpellPoints) override;
	virtual void AddToAttributePoints_Implementation(int32 InAttributePoints) override;
	// End player interface

private:
	virtual void InitAbilityActorInfo() override;
};
