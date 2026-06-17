// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include "D1PlayerState.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UD1LevelupInfo;
class UD1InventoryComponent;
class UD1GameInstance;
struct FD1LoadedStats;
struct FD1LoadedInventoryItem;
struct FD1LoadedEquippedItem;
struct FD1LoadedSkill;
struct FD1LoadedSkillSlot;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStatChanged, int32 /*StatValue*/)

/**
 * 
 */
UCLASS()
class D1_API AD1PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AD1PlayerState();
	void BeginPlay();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Ability System이 이미 초기화되었는지 여부 (ClientTravel 등 맵 이동 시 중복 초기화 방지) */
	UPROPERTY(Replicated)
	bool bAbilitySystemInitialized = false;

	/** 현재 맵에서 Ability 사용(전투/버프/회복 등)이 허용되는가? (GameMode에 의해 설정, Replicated) */
	UPROPERTY(Replicated)
	bool bAbilitiesAllowed = true;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UD1LevelupInfo> LevelUpInfo;

	FOnPlayerStatChanged OnXPChangedDelegate;
	FOnPlayerStatChanged OnLevelChangedDelegate;
	FOnPlayerStatChanged OnAttributePointsChangedDelegate;
	FOnPlayerStatChanged OnSkillPointsChangedDelegate;

	FORCEINLINE int32 GetPlayerLevel() const { return Level; }
	FORCEINLINE int32 GetXP() const { return XP; }
	FORCEINLINE int32 GetAttributePoints() const { return AttributePoints; }
	FORCEINLINE int32 GetSkillPoints() const { return SkillPoints; }

	/** PlayerState에 부착된 인벤토리 컴포넌트를 반환 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UD1InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/**
	 * 파티 시스템에서 사용하는 플레이어 고유 식별자.
	 * 지금: UniqueNetId 문자열 (세션 내 고유). OSS 없는 환경에서는 PlayerName 폴백.
	 * Phase 3: 웹서버가 발급한 캐릭터 ID로 교체 예정 — 이 함수만 수정하면 됨.
	 */
	UFUNCTION(BlueprintPure, Category = "D1|Party")
	FString GetPartyPlayerId() const;

	void AddToXP(int32 InXP);
	void AddToLevel(int32 InLevel);
	void AddToAttributePoints(int32 InPoints);
	void AddToSkillPoints(int32 InPoints);

	void SetXP(int32 InXP);
	void SetLevel(int32 InLevel);

	/**
	 * GameInstance에 저장된 Travel 데이터가 있으면 복원.
	 * 어빌리티 상태는 PendingAbilityRestoreData에 저장되며,
	 * PossessedBy에서 AddCharacterAbilities + UpdateAbilityStatuses 완료 후 RestoreAbilityStates로 적용됨.
	 */
	UFUNCTION(BlueprintCallable, Category = "D1|Travel")
	bool RestoreTravelDataIfNeeded();

	/**
	 * [서버 전용] 웹서버에서 로드한 stats를 적용 (Level/XP/Points + Primary Attribute).
	 * RestoreTravelDataIfNeeded의 DB 버전 — 호출 후 RecalculateSecondaryAttributes 필요.
	 */
	void ApplyLoadedStats(const FD1LoadedStats& Stats);

	/**
	 * [서버 전용] 웹서버에서 로드한 인벤토리/장비를 InventoryComponent에 적용.
	 * DB 타입(문자열 id) → 게임 타입(FName/EEquipmentSlot) 변환 후 RestoreFromSave 호출 (장비 GE 재적용 포함).
	 */
	void ApplyLoadedInventory(const TArray<FD1LoadedInventoryItem>& InInventory,
		const TArray<FD1LoadedEquippedItem>& InEquipped);

	/**
	 * [서버 전용] 웹서버에서 로드한 스킬/스킬슬롯을 ASC에 적용.
	 * DB 데이터를 FD1SavedAbilityInfo로 변환해 RestoreAbilityStates 호출.
	 * ※ 반드시 UpdateAbilityStatuses(레벨 기반 Eligible/Locked 세팅) 이후에 호출할 것.
	 */
	void ApplyLoadedSkills(const TArray<FD1LoadedSkill>& InSkills,
		const TArray<FD1LoadedSkillSlot>& InSkillSlots);

	/** PossessedBy에서 UpdateAbilityStatuses 완료 후 적용할 어빌리티 상태 (서버 전용 임시 필드) */
	TArray<FD1SavedAbilityInfo> PendingAbilityRestoreData;

	/** [서버 전용] 접속 URL 옵션에서 캡처한 세션 토큰. verify-session 호출에 사용. 복제 안 함. */
	FString PendingSessionToken;

	/** [서버 전용] verify-session으로 확정된 웹서버 캐릭터 ID. save 호출에 사용. */
	int64 WebCharacterId = 0;

protected:

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Inventory")
	TObjectPtr<UD1InventoryComponent> InventoryComponent;

private:

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Level)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_XP)
	int32 XP = 1;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_AttributePoints)
	int32 AttributePoints = 5;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_SkillPoints)
	int32 SkillPoints = 1;

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_XP(int32 OldXP);

	UFUNCTION()
	void OnRep_AttributePoints(int32 OldAttributePoints);

	UFUNCTION()
	void OnRep_SkillPoints(int32 OldSkillPoints);
};
