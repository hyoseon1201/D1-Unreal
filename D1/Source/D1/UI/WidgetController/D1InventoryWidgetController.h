// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "Inventory/D1InventoryTypes.h"
#include "D1InventoryWidgetController.generated.h"

class UD1InventoryComponent;
class UD1ItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryUpdatedSignature, const TArray<FD1InventoryItem>&, InventorySlots);

/**
 * 인벤토리 UI (WBP_Inventory)용 위젯 컨트롤러
 * InventoryComponent의 변경사항을 감지하여 UI에 브로드캐스트함
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1InventoryWidgetController : public UD1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	/** 인벤토리 슬롯 배열을 반환 (UI 바인딩용) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FD1InventoryItem>& GetInventorySlots() const;

	/** ItemID로 아이템 메타데이터를 검색 (아이콘, 이름 등) */
	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (DefaultToSelf = "WorldContextObject"))
	static UD1ItemData* GetItemData(const UObject* WorldContextObject, const FName& ItemID);

	/** 인벤토리 변경 시 UI 등에서 수신 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdatedSignature OnInventoryUpdated;

	/** 아이템 사용 요청 (클리언트 -> 서버) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItem(int32 SlotIndex);

	/** 슬롯 이동 요청 (클리언트 -> 서버) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void MoveItem(int32 FromIndex, int32 ToIndex);

	/** 아이템 버리기 요청 (클리언트 -> 서버) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DiscardItem(int32 SlotIndex);

protected:
	/** InventoryComponent의 OnInventoryChanged에 연결될 콜백 */
	UFUNCTION()
	void OnInventoryChangedCallback();

	/** PlayerState로부터 InventoryComponent를 안전하게 획득 */
	UD1InventoryComponent* GetInventoryComponent() const;
};
