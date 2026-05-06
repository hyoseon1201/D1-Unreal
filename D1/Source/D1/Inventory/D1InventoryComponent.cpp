// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/D1InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UD1InventoryComponent::UD1InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	InventorySlots.SetNum(MaxSlots);
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		InventorySlots[i].SlotIndex = i;
	}
}

void UD1InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UD1InventoryComponent, InventorySlots);
}

void UD1InventoryComponent::ServerUseItem_Implementation(int32 SlotIndex)
{
	UseItemInternal(SlotIndex);
}

void UD1InventoryComponent::ServerMoveItem_Implementation(int32 FromIndex, int32 ToIndex)
{
	if (FromIndex < 0 || FromIndex >= MaxSlots || ToIndex < 0 || ToIndex >= MaxSlots)
	{
		return;
	}

	FD1InventoryItem Temp = InventorySlots[FromIndex];
	InventorySlots[FromIndex] = InventorySlots[ToIndex];
	InventorySlots[ToIndex] = Temp;

	InventorySlots[FromIndex].SlotIndex = FromIndex;
	InventorySlots[ToIndex].SlotIndex = ToIndex;

	OnInventoryChanged.Broadcast();
}

void UD1InventoryComponent::ServerDiscardItem_Implementation(int32 SlotIndex)
{
	RemoveItem(SlotIndex, InventorySlots[SlotIndex].Count);
}

bool UD1InventoryComponent::AddItem(const FName& ItemID, int32 Count)
{
	if (ItemID.IsNone() || Count <= 0)
	{
		return false;
	}

	// 빈 슬롯 탐색
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		if (InventorySlots[i].ItemID.IsNone() || InventorySlots[i].Count <= 0)
		{
			InventorySlots[i].ItemID = ItemID;
			InventorySlots[i].Count = Count;
			InventorySlots[i].SlotIndex = i;

			UE_LOG(LogTemp, Warning, TEXT("[AddItem] Slot %d: ItemID=%s, Count=%d"), i, *ItemID.ToString(), Count);
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[AddItem] FAILED: No empty slot for %s"), *ItemID.ToString());
	return false;
}

bool UD1InventoryComponent::RemoveItem(int32 SlotIndex, int32 Count)
{
	if (SlotIndex < 0 || SlotIndex >= MaxSlots || Count <= 0)
	{
		return false;
	}

	FD1InventoryItem& Slot = InventorySlots[SlotIndex];
	if (Slot.ItemID.IsNone() || Slot.Count < Count)
	{
		return false;
	}

	Slot.Count -= Count;
	if (Slot.Count <= 0)
	{
		Slot.ItemID = NAME_None;
		Slot.Count = 0;
		Slot.SlotIndex = SlotIndex;
	}

	OnInventoryChanged.Broadcast();
	return true;
}

void UD1InventoryComponent::OnRep_Inventory()
{
	UE_LOG(LogTemp, Warning, TEXT("[OnRep_Inventory] Slots count=%d"), InventorySlots.Num());
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		if (!InventorySlots[i].ItemID.IsNone())
		{
			UE_LOG(LogTemp, Log, TEXT("  Slot[%d]: ItemID=%s, Count=%d"), i, *InventorySlots[i].ItemID.ToString(), InventorySlots[i].Count);
		}
	}
	OnInventoryChanged.Broadcast();
}

void UD1InventoryComponent::UseItemInternal(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MaxSlots)
	{
		return;
	}

	FD1InventoryItem& Slot = InventorySlots[SlotIndex];
	if (Slot.ItemID.IsNone() || Slot.Count <= 0)
	{
		return;
	}

	// TODO: ItemData 참조하여 실제 효과 적용 (철약, 버프 등)
	UE_LOG(LogTemp, Log, TEXT("[Inventory] UseItem: %s, Count: %d"), *Slot.ItemID.ToString(), Slot.Count);

	RemoveItem(SlotIndex, 1);
}
