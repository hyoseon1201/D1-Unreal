// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/D1InventoryComponent.h"
#include "D1/D1.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/D1ItemData.h"
#include "Inventory/D1ItemRegistry.h"
#include "Game/D1GameStateBase.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "AbilitySystem/D1AttributeSet.h"

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

			UE_LOG(LogD1Inventory, Verbose, TEXT("AddItem: Slot %d <- %s x%d"), i, *ItemID.ToString(), Count);
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	UE_LOG(LogD1Inventory, Warning, TEXT("AddItem FAILED: No empty slot for %s"), *ItemID.ToString());
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
		UE_LOG(LogD1Inventory, Warning, TEXT("UseItemInternal: Owner does not implement IAbilitySystemInterface"));
		return;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("UseItemInternal: ASC is NULL"));
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
		UE_LOG(LogD1Inventory, Warning, TEXT("UseItemInternal: ItemRegistry not found in GameState"));
		return;
	}

	UD1ItemData* ItemData = D1GS->ItemRegistry->FindItemData(Slot.ItemID);
	if (!ItemData)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("UseItemInternal: ItemData not found for %s"), *Slot.ItemID.ToString());
		return;
	}

	/** 소비 아이템만 사용 가능 */
	if (ItemData->ItemType != EItemType::Consumable)
	{
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
			UE_LOG(LogD1Inventory, Verbose, TEXT("UseItemInternal: Applied UseEffect for %s"), *Slot.ItemID.ToString());
		}
	}
	else
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("UseItemInternal: UseEffect is NULL for %s"), *Slot.ItemID.ToString());
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
	if (InventorySlotIndex < 0 || InventorySlotIndex >= MaxSlots)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: Invalid slot index %d (MaxSlots=%d)"), InventorySlotIndex, MaxSlots);
		return;
	}

	FD1InventoryItem& Slot = InventorySlots[InventorySlotIndex];
	if (Slot.ItemID.IsNone() || Slot.Count <= 0)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: Slot %d is empty"), InventorySlotIndex);
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	/** ItemRegistry에서 메타데이터 룩업 */
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AD1GameStateBase* D1GS = Cast<AD1GameStateBase>(World->GetGameState());
	if (!D1GS || !D1GS->ItemRegistry)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: ItemRegistry not found in GameState"));
		return;
	}

	UD1ItemData* ItemData = D1GS->ItemRegistry->FindItemData(Slot.ItemID);
	if (!ItemData)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: ItemData not found for %s"), *Slot.ItemID.ToString());
		return;
	}

	/** 장비 아이템인지 확인 */
	if (ItemData->ItemType != EItemType::Equipment)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: %s is not Equipment (Type=%d)"), *Slot.ItemID.ToString(), (int32)ItemData->ItemType);
		return;
	}

	EEquipmentSlot TargetSlot = ItemData->EquipmentSlot;
	if (TargetSlot == EEquipmentSlot::None)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: EquipmentSlot is None for %s"), *Slot.ItemID.ToString());
		return;
	}

	/** 해당 부위에 이미 장비가 있으면 탈착 */
	if (IsSlotEquipped(TargetSlot))
	{
		UnequipItemInternal(TargetSlot);
	}

	/** ASC 획득 */
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
	if (!ASCInterface)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: Owner does not implement IAbilitySystemInterface"));
		return;
	}
	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem FAILED: ASC is NULL"));
		return;
	}

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

			// 진단: GE 적용 직후 (RecalcSecondary 전) AttackPower
			float DBG_AP_AfterGE = -1.f;
			if (const UD1AttributeSet* AS = Cast<UD1AttributeSet>(ASC->GetAttributeSet(UD1AttributeSet::StaticClass())))
				DBG_AP_AfterGE = AS->GetAttackPower();
			UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem: GE applied (Handle valid=%d, GE Instant=%d). AttackPower=%.0f"),
				ActiveHandle.IsValid(),
				!ActiveHandle.IsValid(),  // Instant GE는 소비되어 Handle이 Invalid
				DBG_AP_AfterGE);
		}
		else
		{
			UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem: SpecHandle is invalid for %s"), *Slot.ItemID.ToString());
		}
	}
	else
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem: EquipEffect is NULL for %s"), *Slot.ItemID.ToString());
	}

	/** 장착 목록에 추가 */
	FD1EquippedItem EquippedItem;
	EquippedItem.EquipmentSlot = TargetSlot;
	EquippedItem.Item = Slot;
	EquippedItem.Item.SlotIndex = -1;
	EquippedItems.Add(EquippedItem);

	/** 인벤토리에서 제거 */
	RemoveItem(InventorySlotIndex, 1);

	{
		float DBG_AP = -1.f;
		if (const UD1AttributeSet* AS = Cast<UD1AttributeSet>(ASC->GetAttributeSet(UD1AttributeSet::StaticClass())))
			DBG_AP = AS->GetAttackPower();
		UE_LOG(LogD1Inventory, Warning, TEXT("EquipItem: %s equipped. AttackPower=%.0f"), *EquippedItem.Item.ItemID.ToString(), DBG_AP);
	}

	UE_LOG(LogD1Inventory, Verbose, TEXT("EquipItem: %s equipped on slot %d. Total equipped=%d"),
		*EquippedItem.Item.ItemID.ToString(), (int32)TargetSlot, EquippedItems.Num());

	OnEquippedItemsChanged.Broadcast();
}

void UD1InventoryComponent::UnequipItemInternal(EEquipmentSlot Slot)
{
	if (Slot == EEquipmentSlot::None)
	{
		return;
	}

	const FD1InventoryItem* EquippedItem = FindEquippedItem(Slot);
	if (!EquippedItem)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("UnequipItem FAILED: No item equipped on slot %d"), (int32)Slot);
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	/** ASC 획득 및 GE 제거 */
	UAbilitySystemComponent* ASC = nullptr;
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
	if (ASCInterface)
	{
		ASC = ASCInterface->GetAbilitySystemComponent();
		if (ASC && EquippedEffectHandles.Contains(Slot))
		{
			ASC->RemoveActiveGameplayEffect(EquippedEffectHandles[Slot]);
		}
		else
		{
			UE_LOG(LogD1Inventory, Warning, TEXT("UnequipItem: ASC or EquipEffect Handle not found for slot %d"), (int32)Slot);
		}
	}
	EquippedEffectHandles.Remove(Slot);

	/** 인벤토리에 되돌리기 (빈 슬롯이 없으면 실패) */
	if (!AddItem(EquippedItem->ItemID, EquippedItem->Count))
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("UnequipItem: Inventory full! Dropping item %s"), *EquippedItem->ItemID.ToString());
		// TODO: 월드에 아이템 드랍 처리 (Phase 2)
	}

	/** 장착 목록에서 제거 */
	for (int32 i = EquippedItems.Num() - 1; i >= 0; --i)
	{
		if (EquippedItems[i].EquipmentSlot == Slot)
		{
			EquippedItems.RemoveAt(i);
			break;
		}
	}

	UE_LOG(LogD1Inventory, Verbose, TEXT("UnequipItem: slot %d unequipped. Total equipped=%d"), (int32)Slot, EquippedItems.Num());

	OnEquippedItemsChanged.Broadcast();
}


void UD1InventoryComponent::RestoreFromSave(const TArray<FD1InventoryItem>& InInventorySlots, const TArray<FD1EquippedItem>& InEquippedItems)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 인벤토리 슬롯 복원 (슬롯 수 불일치 대비)
	InventorySlots.SetNum(MaxSlots);
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		InventorySlots[i] = (i < InInventorySlots.Num()) ? InInventorySlots[i] : FD1InventoryItem{};
		InventorySlots[i].SlotIndex = i;
	}

	// 장착 목록 복원
	EquippedItems = InEquippedItems;
	EquippedEffectHandles.Empty();

	// 장착 GE 재적용
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
	if (!ASCInterface)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("RestoreFromSave: Owner has no ASC interface"));
	}
	else if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
	{
		AD1GameStateBase* D1GS = Cast<AD1GameStateBase>(GetWorld()->GetGameState());
		if (D1GS && D1GS->ItemRegistry)
		{
			for (const FD1EquippedItem& EquippedItem : EquippedItems)
			{
				UD1ItemData* ItemData = D1GS->ItemRegistry->FindItemData(EquippedItem.Item.ItemID);
				if (!ItemData || !ItemData->EquipEffect)
				{
					continue;
				}

				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				EffectContext.AddSourceObject(Owner);
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemData->EquipEffect, 1.0f, EffectContext);
				if (SpecHandle.IsValid())
				{
					FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					EquippedEffectHandles.Add(EquippedItem.EquipmentSlot, ActiveHandle);
					UE_LOG(LogD1Inventory, Log, TEXT("RestoreFromSave: Re-applied GE for %s (Handle valid=%d)"),
						*EquippedItem.Item.ItemID.ToString(), ActiveHandle.IsValid());
				}
			}
		}
	}

	UE_LOG(LogD1Inventory, Log, TEXT("RestoreFromSave: Inventory=%d, Equipped=%d"), InventorySlots.Num(), EquippedItems.Num());

	OnInventoryChanged.Broadcast();
	OnEquippedItemsChanged.Broadcast();
}

void UD1InventoryComponent::OnRep_EquippedItems()
{
	OnEquippedItemsChanged.Broadcast();
}
