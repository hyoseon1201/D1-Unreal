// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1InventoryWidgetController.h"
#include "Player/D1PlayerState.h"
#include "Inventory/D1InventoryComponent.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"

void UD1InventoryWidgetController::BroadcastInitialValues()
{
	// 위젯이 생성될 때 현재 인벤토리 상태를 한 번 브로드캐스트
	OnInventoryChangedCallback();
}

void UD1InventoryWidgetController::BindCallbacksToDependencies()
{
	// InventoryComponent의 변경 델리게이트에 연결
	if (UD1InventoryComponent* IC = GetInventoryComponent())
	{
		IC->OnInventoryChanged.AddDynamic(this, &UD1InventoryWidgetController::OnInventoryChangedCallback);
	}
}

const TArray<FD1InventoryItem>& UD1InventoryWidgetController::GetInventorySlots() const
{
	if (UD1InventoryComponent* IC = GetInventoryComponent())
	{
		return IC->GetInventorySlots();
	}

	// InventoryComponent가 없으면 빈 배열 반환
	static TArray<FD1InventoryItem> EmptyArray;
	return EmptyArray;
}

UD1ItemData* UD1InventoryWidgetController::GetItemData(const UObject* WorldContextObject, const FName& ItemID)
{
	return UD1AbilitySystemLibrary::GetItemData(WorldContextObject, ItemID);
}

void UD1InventoryWidgetController::UseItem(int32 SlotIndex)
{
	if (UD1InventoryComponent* IC = GetInventoryComponent())
	{
		IC->ServerUseItem(SlotIndex);
	}
}

void UD1InventoryWidgetController::MoveItem(int32 FromIndex, int32 ToIndex)
{
	if (UD1InventoryComponent* IC = GetInventoryComponent())
	{
		IC->ServerMoveItem(FromIndex, ToIndex);
	}
}

void UD1InventoryWidgetController::DiscardItem(int32 SlotIndex)
{
	if (UD1InventoryComponent* IC = GetInventoryComponent())
	{
		IC->ServerDiscardItem(SlotIndex);
	}
}

void UD1InventoryWidgetController::OnInventoryChangedCallback()
{
	if (UD1InventoryComponent* IC = GetInventoryComponent())
	{
		OnInventoryUpdated.Broadcast(IC->GetInventorySlots());
	}
}

UD1InventoryComponent* UD1InventoryWidgetController::GetInventoryComponent() const
{
	if (AD1PlayerState* PS = Cast<AD1PlayerState>(PlayerState))
	{
		return PS->GetInventoryComponent();
	}
	return nullptr;
}
