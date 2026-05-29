// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "D1DungeonResultWidgetController.generated.h"

/** 델리게이트: 획득 아이템 목록 변경 시 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAcquiredItemsChanged, const TArray<FText>&, Items);

/**
 * 던전 클리어 결과 위젯의 데이터를 관리하는 WidgetController.
 * 획득 아이템 목록 등을 보관하고 Blueprint 위젯에 전달.
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1DungeonResultWidgetController : public UD1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;

	/** 획득한 아이템 목록을 설정 (던전 클리어 시 GameMode에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Dungeon Result")
	void SetAcquiredItems(const TArray<FText>& InItems);

	/** 획득 아이템 목록이 변경될 때 Blueprint에 알림 */
	UPROPERTY(BlueprintAssignable, Category = "Dungeon Result")
	FOnAcquiredItemsChanged OnAcquiredItemsChanged;

protected:
	/** 획득한 아이템 이름 목록 */
	UPROPERTY(BlueprintReadOnly, Category = "Dungeon Result")
	TArray<FText> AcquiredItems;
};
