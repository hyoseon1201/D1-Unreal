// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/D1GameModeBase.h"
#include "D1GameModeTown.generated.h"

/**
 * 
 */
UCLASS()
class D1_API AD1GameModeTown : public AD1GameModeBase
{
	GENERATED_BODY()

public:
	/** 마을에서는 전투/버프/회복 등 모든 Ability 사용 불가 */
	virtual bool AreAbilitiesAllowed() const override { return false; }
};
