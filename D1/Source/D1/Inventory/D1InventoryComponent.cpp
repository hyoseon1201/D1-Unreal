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
