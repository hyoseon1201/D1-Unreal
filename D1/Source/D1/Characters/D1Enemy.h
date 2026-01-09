// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/D1CharacterBase.h"
#include "D1Enemy.generated.h"

/**
 * 
 */
UCLASS()
class D1_API AD1Enemy : public AD1CharacterBase
{
	GENERATED_BODY()
	
public:
	AD1Enemy();

	virtual void PossessedBy(AController* NewController) override;

	// combat interface
	virtual int32 GetLevel() const override;

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Class Defaults")
	int32 Level = 1;
};
