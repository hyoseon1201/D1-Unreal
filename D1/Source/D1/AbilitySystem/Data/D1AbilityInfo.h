// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "D1AbilityInfo.generated.h"

USTRUCT(BlueprintType)
struct FD1AbilityTagInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag InputTag = FGameplayTag();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UTexture2D> Icon = nullptr;
};

/**
 * 
 */
UCLASS()
class D1_API UD1AbilityInfo : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray< FD1AbilityTagInfo> AbilityInformation;

	FD1AbilityTagInfo FindAbilityTagInforTag(const FGameplayTag& AbilityTag, bool bLogNotFound = false) const;
};
