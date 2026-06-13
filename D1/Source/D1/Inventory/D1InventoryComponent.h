// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/D1InventoryTypes.h"
#include "GameplayEffectTypes.h"
#include "D1InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquippedItemsChanged);

/**
 * PlayerState에 부착되는 인벤토리 컴포넌트
 * 모든 로직은 서버에서 실행되며 클라이언트는 Replication으로 동기화
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class D1_API UD1InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UD1InventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 인벤토리 변경 시 UI 등에서 수신 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/** 장착 장비 변경 시 UI 등에서 수신 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	/** 서버 RPC: 아이템 사용 */
	UFUNCTION(Server, Reliable)
	void ServerUseItem(int32 SlotIndex);

	/** 서버 RPC: 슬롯 이동 */
	UFUNCTION(Server, Reliable)
	void ServerMoveItem(int32 FromIndex, int32 ToIndex);

	/** 서버 RPC: 아이템 버리기 */
	UFUNCTION(Server, Reliable)
	void ServerDiscardItem(int32 SlotIndex);

	/** 서버 RPC: 장비 장착 */
	UFUNCTION(Server, Reliable)
	void ServerEquipItem(int32 InventorySlotIndex);

	/** 서버 RPC: 장비 탈착 */
	UFUNCTION(Server, Reliable)
	void ServerUnequipItem(EEquipmentSlot Slot);

	/** 서버 전용: 아이템 추가 (보상, 획득 등) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FName& ItemID, int32 Count = 1);

	/** 서버 전용: 아이템 제거 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 SlotIndex, int32 Count = 1);

	/** 현재 인벤토리 슬롯 배열 읽기 (클리언트용) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FD1InventoryItem>& GetInventorySlots() const { return InventorySlots; }

	/** 현재 장착 중인 장비 목록 읽기 (클리언트용) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FD1EquippedItem>& GetEquippedItems() const { return EquippedItems; }

	/** 특정 부위에 장비가 장착되어 있는지 확인 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEquipped(EEquipmentSlot Slot) const;

	/** 특정 부위의 장착 아이템을 검색 (없으면 nullptr) */
	const FD1InventoryItem* FindEquippedItem(EEquipmentSlot Slot) const;

	/** 맵 이동 후 저장된 인벤토리/장비 데이터를 복원하고 장비 GE를 재적용 (서버 전용) */
	void RestoreFromSave(const TArray<FD1InventoryItem>& InInventorySlots, const TArray<FD1EquippedItem>& InEquippedItems);

protected:
	/** 최대 슬롯 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 MaxSlots = 20;

	/** 인벤토리 슬롯 배열 (서버 권한, Replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	TArray<FD1InventoryItem> InventorySlots;

	/** 장착 중인 장비 (서버 권한, Replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems)
	TArray<FD1EquippedItem> EquippedItems;

	/** 서버 전용: 장착된 GE의 Active Handle (탈착 시 제거용) */
	TMap<EEquipmentSlot, FActiveGameplayEffectHandle> EquippedEffectHandles;

	/** 클라이언트 동기화 시 호출 */
	UFUNCTION()
	void OnRep_Inventory();

	/** 클라이언트 동기화 시 호출 */
	UFUNCTION()
	void OnRep_EquippedItems();

	/** 아이템 사용 실제 로직 (서버 전용) */
	void UseItemInternal(int32 SlotIndex);

	/** 장비 장착 실제 로직 (서버 전용) */
	void EquipItemInternal(int32 InventorySlotIndex);

	/** 장비 탈착 실제 로직 (서버 전용) */
	void UnequipItemInternal(EEquipmentSlot Slot);
};
