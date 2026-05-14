// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/D1InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/D1ItemData.h"
#include "Inventory/D1ItemRegistry.h"
#include "Game/D1GameStateBase.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

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
	DOREPLIFETIME(UD1InventoryComponent, EquippedItems);
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

	/** 서버에서만 실행 (RPC를 통해 호출되므로 원래 서버지만 안전 장치) */
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	/** 소유자(PlayerState)의 ASC 획득 */
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
	if (!ASCInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UseItemInternal] Owner does not implement IAbilitySystemInterface"));
		return;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UseItemInternal] ASC is NULL"));
		return;
	}

	/** GameState에서 ItemRegistry를 통해 메타데이터 룩업 */
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	AD1GameStateBase* D1GS = Cast<AD1GameStateBase>(GameState);
	if (!D1GS || !D1GS->ItemRegistry)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UseItemInternal] ItemRegistry not found in GameState"));
		return;
	}

	UD1ItemData* ItemData = D1GS->ItemRegistry->FindItemData(Slot.ItemID);
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UseItemInternal] ItemData not found for %s"), *Slot.ItemID.ToString());
		return;
	}

	/** 소비 아이템만 사용 가능 */
	if (ItemData->ItemType != EItemType::Consumable)
	{
		UE_LOG(LogTemp, Log, TEXT("[UseItemInternal] %s is not consumable (Type=%d)"), *Slot.ItemID.ToString(), (int32)ItemData->ItemType);
		return;
	}

	/** UseEffect GE 적용 */
	if (ItemData->UseEffect)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(Owner);

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemData->UseEffect, 1.0f, EffectContext);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			UE_LOG(LogTemp, Log, TEXT("[UseItemInternal] Applied UseEffect for %s"), *Slot.ItemID.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[UseItemInternal] UseEffect is NULL for %s"), *Slot.ItemID.ToString());
	}

	/** 아이템 1개 소모 */
	RemoveItem(SlotIndex, 1);
}

void UD1InventoryComponent::ServerEquipItem_Implementation(int32 InventorySlotIndex)
{
	EquipItemInternal(InventorySlotIndex);
}

void UD1InventoryComponent::ServerUnequipItem_Implementation(EEquipmentSlot Slot)
{
	UnequipItemInternal(Slot);
}

bool UD1InventoryComponent::IsSlotEquipped(EEquipmentSlot Slot) const
{
	for (const FD1EquippedItem& Equipped : EquippedItems)
	{
		if (Equipped.EquipmentSlot == Slot)
		{
			return true;
		}
	}
	return false;
}

const FD1InventoryItem* UD1InventoryComponent::FindEquippedItem(EEquipmentSlot Slot) const
{
	for (const FD1EquippedItem& Equipped : EquippedItems)
	{
		if (Equipped.EquipmentSlot == Slot)
		{
			return &Equipped.Item;
		}
	}
	return nullptr;
}

void UD1InventoryComponent::EquipItemInternal(int32 InventorySlotIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] START SlotIndex=%d"), InventorySlotIndex);
	
	if (InventorySlotIndex < 0 || InventorySlotIndex >= MaxSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: Invalid slot index %d (MaxSlots=%d)"), InventorySlotIndex, MaxSlots);
		return;
	}

	FD1InventoryItem& Slot = InventorySlots[InventorySlotIndex];
	UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] Slot[%d]: ItemID=%s, Count=%d"), InventorySlotIndex, *Slot.ItemID.ToString(), Slot.Count);
	
	if (Slot.ItemID.IsNone() || Slot.Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: Slot is empty"));
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: No authority"));
		return;
	}

	/** ItemRegistry에서 메타데이터 룩업 */
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: World is NULL"));
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: GameState is NULL"));
		return;
	}

	AD1GameStateBase* D1GS = Cast<AD1GameStateBase>(GameState);
	if (!D1GS || !D1GS->ItemRegistry)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: D1GameStateBase or ItemRegistry is NULL"));
		return;
	}

	UD1ItemData* ItemData = D1GS->ItemRegistry->FindItemData(Slot.ItemID);
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: ItemData not found for %s"), *Slot.ItemID.ToString());
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] Found ItemData: Name=%s, Type=%d, EquipSlot=%d"), 
		*ItemData->ItemName.ToString(), (int32)ItemData->ItemType, (int32)ItemData->EquipmentSlot);

	/** 장비 아이템인지 확인 */
	if (ItemData->ItemType != EItemType::Equipment)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: %s is not Equipment (Type=%d)"), *Slot.ItemID.ToString(), (int32)ItemData->ItemType);
		return;
	}

	EEquipmentSlot TargetSlot = ItemData->EquipmentSlot;
	if (TargetSlot == EEquipmentSlot::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: EquipmentSlot is None for %s"), *Slot.ItemID.ToString());
		return;
	}

	/** 해당 부위에 이미 장비가 있으면 탈착 */
	if (IsSlotEquipped(TargetSlot))
	{
		UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] Slot %d already equipped, unequipping first"), (int32)TargetSlot);
		UnequipItemInternal(TargetSlot);
	}

	/** ASC 획득 */
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
	if (!ASCInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: Owner does not implement IAbilitySystemInterface"));
		return;
	}
	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: ASC is NULL"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] ASC found on %s"), *Owner->GetName());

	/** EquipEffect GE 적용 */
	if (ItemData->EquipEffect)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(Owner);

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemData->EquipEffect, 1.0f, EffectContext);
		if (SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			EquippedEffectHandles.Add(TargetSlot, ActiveHandle);
			UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] SUCCESS: Applied EquipEffect for %s on slot %d"), 
				*Slot.ItemID.ToString(), (int32)TargetSlot);
			
			/** 적용된 Attribute 변화 확인 */
			if (ItemData->EquipEffect)
			{
				UGameplayEffect* EffectCDO = ItemData->EquipEffect.GetDefaultObject();
				if (EffectCDO)
				{
					UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] GE Modifiers count: %d"), EffectCDO->Modifiers.Num());
					for (const FGameplayModifierInfo& Mod : EffectCDO->Modifiers)
					{
						UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] GE Modifier: Attribute=%s, Op=%d"),
							*Mod.Attribute.GetName(), (int32)Mod.ModifierOp);
					}
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: SpecHandle is invalid for %s"), *Slot.ItemID.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipItemInternal] FAILED: EquipEffect is NULL for %s"), *Slot.ItemID.ToString());
	}

	/** 장착 목록에 추가 */
	FD1EquippedItem EquippedItem;
	EquippedItem.EquipmentSlot = TargetSlot;
	EquippedItem.Item = Slot;
	EquippedItem.Item.SlotIndex = -1;
	EquippedItems.Add(EquippedItem);

	/** 인벤토리에서 제거 */
	RemoveItem(InventorySlotIndex, 1);
	
	UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] END: Added to EquippedItems, removed from inventory. Total equipped=%d"), EquippedItems.Num());

	/** 현재 장착 상태 로그 출력 */
	UE_LOG(LogTemp, Log, TEXT("[EquipItemInternal] === Current Equipped Items ==="));
	if (EquippedItems.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("  (No equipped items)"));
	}
	else
	{
		for (const FD1EquippedItem& Equipped : EquippedItems)
		{
			UE_LOG(LogTemp, Log, TEXT("  Slot[%d]: ItemID=%s, Count=%d"),
				(int32)Equipped.EquipmentSlot, *Equipped.Item.ItemID.ToString(), Equipped.Item.Count);
		}
	}

	OnEquippedItemsChanged.Broadcast();
}

void UD1InventoryComponent::UnequipItemInternal(EEquipmentSlot Slot)
{
	UE_LOG(LogTemp, Log, TEXT("[UnequipItemInternal] START Slot=%d"), (int32)Slot);
	
	if (Slot == EEquipmentSlot::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UnequipItemInternal] FAILED: Slot is None"));
		return;
	}

	const FD1InventoryItem* EquippedItem = FindEquippedItem(Slot);
	if (!EquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UnequipItemInternal] FAILED: No item equipped on slot %d"), (int32)Slot);
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[UnequipItemInternal] Found equipped item: %s"), *EquippedItem->ItemID.ToString());

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UnequipItemInternal] FAILED: No authority"));
		return;
	}

	/** ASC 획득 및 GE 제거 */
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
	if (ASCInterface)
	{
		UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
		if (ASC && EquippedEffectHandles.Contains(Slot))
		{
			FActiveGameplayEffectHandle Handle = EquippedEffectHandles[Slot];
			ASC->RemoveActiveGameplayEffect(Handle);
			UE_LOG(LogTemp, Log, TEXT("[UnequipItemInternal] SUCCESS: Removed EquipEffect Handle for %s from slot %d"), 
				*EquippedItem->ItemID.ToString(), (int32)Slot);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[UnequipItemInternal] WARNING: ASC or Handle not found for slot %d"), (int32)Slot);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[UnequipItemInternal] WARNING: Owner does not implement IAbilitySystemInterface"));
	}
	EquippedEffectHandles.Remove(Slot);

	/** 인벤토리에 되돌리기 (빈 슬롯이 없으면 실패) */
	if (!AddItem(EquippedItem->ItemID, EquippedItem->Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UnequipItemInternal] Inventory full! Dropping item %s"), *EquippedItem->ItemID.ToString());
		// TODO: 월드에 아이템 드랍 처리 (Phase 2)
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[UnequipItemInternal] Item returned to inventory: %s"), *EquippedItem->ItemID.ToString());
	}

	/** 장착 목록에서 제거 */
	for (int32 i = EquippedItems.Num() - 1; i >= 0; --i)
	{
		if (EquippedItems[i].EquipmentSlot == Slot)
		{
			EquippedItems.RemoveAt(i);
			UE_LOG(LogTemp, Log, TEXT("[UnequipItemInternal] Removed from EquippedItems array"));
			break;
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("[UnequipItemInternal] END: Total equipped=%d"), EquippedItems.Num());

	OnEquippedItemsChanged.Broadcast();
}

void UD1InventoryComponent::OnRep_EquippedItems()
{
	OnEquippedItemsChanged.Broadcast();
}
