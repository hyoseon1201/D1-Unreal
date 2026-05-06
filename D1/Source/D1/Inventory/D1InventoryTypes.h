// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "D1InventoryTypes.generated.h"

/**
 * 아이템 대분류
 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	None		UMETA(DisplayName = "None"),
	Consumable	UMETA(DisplayName = "소비 아이템"),
	Equipment	UMETA(DisplayName = "장비 아이템"),
	Material	UMETA(DisplayName = "재료"),
	Quest		UMETA(DisplayName = "퀘스트 아이템"),
	Etc			UMETA(DisplayName = "기타"),
};

/**
 * 장비 착용 부위
 */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None		UMETA(DisplayName = "None"),
	Weapon		UMETA(DisplayName = "무기"),
	Helmet		UMETA(DisplayName = "투구"),
	Armor		UMETA(DisplayName = "갑옷"),
	Gloves		UMETA(DisplayName = "장갑"),
	Boots		UMETA(DisplayName = "신발"),
	Necklace	UMETA(DisplayName = "목걸이"),
	Ring		UMETA(DisplayName = "반지"),
};

/**
 * 인벤토리 슬롯 아이템 데이터
 */
USTRUCT(BlueprintType)
struct FD1InventoryItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName ItemID;

	UPROPERTY(BlueprintReadOnly)
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = -1;
};
