// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "D1LevelupInfo.generated.h"

USTRUCT(BlueprintType)
struct FD1LevelupTagInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	int32 LevelupRequirement = 0;

	UPROPERTY(EditDefaultsOnly)
	int32 AttributePointAward = 1;

	UPROPERTY(EditDefaultsOnly)
	int32 SpellPointAward = 1;
};

/**
 * 
 */
UCLASS()
class D1_API UD1LevelupInfo : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FD1LevelupTagInfo> LevelupInformation;

	int32 FindLevelForXP(int32 XP) const;
};
