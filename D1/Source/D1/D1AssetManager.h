// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "D1AssetManager.generated.h"

/**
 * 
 */
UCLASS()
class D1_API UD1AssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	static UD1AssetManager& Get();

protected:
	virtual void StartInitialLoading() override;
};
