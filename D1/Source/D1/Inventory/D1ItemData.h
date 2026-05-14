// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/D1InventoryTypes.h"
#include "GameplayEffect.h"
#include "D1ItemData.generated.h"

/**
 * 개별 아이템의 정적 메타데이터
 * 에디터에서 각 아이템별 .uasset 인스턴스를 생성하여 사용
 */
UCLASS(BlueprintType)
class D1_API UD1ItemData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 아이템 고유 ID (FName 키, InventoryItem.ItemID 와 매칭) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Meta")
	FName ItemID;

	/** 아이템 표시 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Meta")
	FText ItemName;

	/** 아이템 설명 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Meta")
	FText Description;

	/** 아이템 대분류 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Meta")
	EItemType ItemType = EItemType::None;

	/** 장비 착용 부위 (ItemType == Equipment 일 때만 편집 가능) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Meta",
		meta = (EditCondition = "ItemType == EItemType::Equipment", EditConditionHides))
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

	/** 최대 중첩 개수 (장비는 1, 포션 등은 99 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Meta")
	int32 MaxStack = 1;

	/** 아이콘 텍스처 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Visual")
	TObjectPtr<UTexture2D> Icon;

	// --- 장비 아이템 전용 ---

	/** 장착 시 ASC에 적용할 GameplayEffect (스탯 상승 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",
		meta = (EditCondition = "ItemType == EItemType::Equipment", EditConditionHides))
	TSubclassOf<UGameplayEffect> EquipEffect;

	// --- 소비 아이템 전용 ---

	/** 사용 시 ASC에 적용할 GameplayEffect (회복, 버프 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable",
		meta = (EditCondition = "ItemType == EItemType::Consumable", EditConditionHides))
	TSubclassOf<UGameplayEffect> UseEffect;

	// --- 거래 ---

	/** 판매 가격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
	int32 SellPrice = 0;

	/** 구매 가격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
	int32 BuyPrice = 0;
};
