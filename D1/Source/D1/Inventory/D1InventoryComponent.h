// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/D1InventoryTypes.h"
#include "D1InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

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

	/** 서버 RPC: 아이템 사용 */
	UFUNCTION(Server, Reliable)
	void ServerUseItem(int32 SlotIndex);

	/** 서버 RPC: 슬롯 이동 */
	UFUNCTION(Server, Reliable)
	void ServerMoveItem(int32 FromIndex, int32 ToIndex);

	/** 서버 RPC: 아이템 버리기 */
	UFUNCTION(Server, Reliable)
	void ServerDiscardItem(int32 SlotIndex);

	/** 서버 전용: 아이템 추가 (보상, 획득 등) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FName& ItemID, int32 Count = 1);

	/** 서버 전용: 아이템 제거 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 SlotIndex, int32 Count = 1);

	/** 현재 인벤토리 슬롯 배열 읽기 (클리언트용) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FD1InventoryItem>& GetInventorySlots() const { return InventorySlots; }

protected:
	/** 최대 슬롯 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 MaxSlots = 20;

	/** 인벤토리 슬롯 배열 (서버 권한, Replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	TArray<FD1InventoryItem> InventorySlots;

	/** 클라이언트 동기화 시 호출 */
	UFUNCTION()
	void OnRep_Inventory();

	/** 아이템 사용 실제 로직 (서버 전용) */
	void UseItemInternal(int32 SlotIndex);
};
