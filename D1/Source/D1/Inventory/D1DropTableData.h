// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "D1DropTableData.generated.h"

class UD1ItemData;

/** 드롭 테이블 항목 1개 (아이템 + 확률 + 수량 범위) */
USTRUCT(BlueprintType)
struct FD1DropEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	TObjectPtr<UD1ItemData> ItemData;

	/** 드롭 확률 (0~100) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop", meta = (ClampMin = 0.f, ClampMax = 100.f))
	float DropChance = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop", meta = (ClampMin = 1))
	int32 MinQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop", meta = (ClampMin = 1))
	int32 MaxQuantity = 1;
};

/**
 * 몬스터 1종에 붙이는 드롭 테이블 DataAsset.
 * 에디터에서 DA_DropTable_Goblin 등 인스턴스를 만들어 Enemy BP에 할당.
 */
UCLASS(BlueprintType)
class D1_API UD1DropTableData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	TArray<FD1DropEntry> Entries;
};
