// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/D1ItemData.h"
#include "D1ItemRegistry.generated.h"

/**
 * 모든 아이템 DataAsset을 등록하고 룩업하는 레지스트리
 * GameMode에서 참조하여 런타임에 ItemID로 UD1ItemData를 검색함
 */
UCLASS()
class D1_API UD1ItemRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	/** ItemID -> UD1ItemData 매핑 테이블 (에디터에서 편집) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Registry")
	TMap<FName, TObjectPtr<UD1ItemData>> Items;

	/** ItemID로 아이템 데이터를 검색 (없으면 nullptr) */
	UFUNCTION(BlueprintCallable, Category = "Item Registry")
	UD1ItemData* FindItemData(const FName& ItemID) const;
};
