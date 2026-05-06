# D1 아이템 데이터 기획서 (Item Data Reference)

> **목적:** 아이템 분류, 장비 부위, 효과 체계 및 C++ 데이터 구조 정의  
> **대상 독자:** 클라이언트/서버 개발자 및 웹서버 개발자  
> **기준 시점:** 2026-05-03 — 인벤토리 시스템 Phase 1 진행 중

---

## 목차

1. [아이템 분류 (EItemType)](#1-아이템-분류-eitemtype)
2. [장비 부위 (EEquipmentSlot)](#2-장비-부위-eequipmentslot)
3. [C++ 데이터 구조](#3-c-데이터-구조)
4. [아이템 효과 설계 (GameplayEffect 연동)](#4-아이템-효과-설계-gameplayeffect-연동)
5. [예시 아이템 정의](#5-예시-아이템-정의)
6. [인벤토리 연동 흐름](#6-인벤토리-연동-흐름)
7. [웹서버 연동 시 고려사항](#7-웹서버-연동-시-고려사항)
8. [TODO / 확장 아이디어](#8-todo--확장-아이디어)

---

## 1. 아이템 분류 (EItemType)

```cpp
UENUM(BlueprintType)
enum class EItemType : uint8
{
    None,        // 미분류 (기본값, 사용 금지 권장)
    Consumable,  // 소비 아이템: 포션, 스크롤, 음식 등 (1회 또는 지속 효과)
    Equipment,   // 장비 아이템: 무기, 방어구, 악세서리 (장착 시 스탯 변동)
    Material,    // 재료: 제작, 강화, 합성용
    Quest,       // 퀘스트 아이템: 특정 퀘스트 진행용 (거래/버리기 불가)
    Etc,         // 기타: 이벤트 아이템, 기프트 박스 등
};
```

| 분류 | 코드 | 특징 | 예시 |
|------|------|------|------|
| **소비** | `Consumable` | 사용 시 소멸 또는 개수 차감, `UseEffect` GE 적용 | 체력 포션, 마나 포션, 버프 스크롤 |
| **장비** | `Equipment` | `EquipmentSlot` 부위 지정, `EquipEffect` GE 적용/해제 | 검, 투구, 갑옷, 반지 |
| **재료** | `Material` | 중첩 가능, 제작/강화 시 소모 | 철광석, 가죽, 마법 가루 |
| **퀘스트** | `Quest` | 퀘스트 진행에 필수, 일반 거래 불가 | 고블린의 귀, 왕의 편지 |
| **기타** | `Etc` | 자유롭게 확장 가능 | 이벤트 쿠폰, 상자 |

> **인벤토리 UI 기획 팁:** `ItemType`별로 탭(소비/장비/재료/퀘스트)을 나누거나, 아이콘 테두리 색상을 다르게 표시하면 가독성이 좋아짐.

---

## 2. 장비 부위 (EEquipmentSlot)

```cpp
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    None,     // 미장착/미분류
    Weapon,   // 무기 (1개)
    Helmet,   // 투구 (1개)
    Armor,    // 갑옷/상의 (1개)
    Gloves,   // 장갑 (1개)
    Boots,    // 신발 (1개)
    Necklace, // 목걸이 (1개)
    Ring,     // 반지 (2개 슬롯 확장 가능)
};
```

| 부위 | 코드 | 장착 개수 | 주요 스탯 방향 | 비고 |
|------|------|----------|---------------|------|
| **무기** | `Weapon` | 1 | 공격력, 치명타 | 근거리/원거리/마법 무기 세분화는 `ItemID` 네이밍 또는 추가 태그로 처리 |
| **투구** | `Helmet` | 1 | 최대 체력, 체력 재생 | |
| **갑옷** | `Armor` | 1 | 방어력, 최대 체력 | |
| **장갑** | `Gloves` | 1 | 공격 속도(추후), 치명타 확률 | |
| **신발** | `Boots` | 1 | 이동 속도(추후), 회피(추후) | |
| **목걸이** | `Necklace` | 1 | 마나, 지능 | |
| **반지** | `Ring` | 1 (확장 시 2) | 다양한 보조 스탯 | 양손 반지 슬롯 분리는 Phase 2 이후 고려 |

> **주의:** `EEquipmentSlot`은 `ItemType == Equipment`일 때만 유효함. 에디터에서 `EditCondition`으로 자동 숨김 처리됨.

---

## 3. C++ 데이터 구조

### 3.1 `UD1ItemData` — 개별 아이템 메타데이터

> 위치: `Source/D1/Inventory/D1ItemData.h`  
> 상속: `UDataAsset`

개별 아이템당 하나의 `.uasset` 인스턴스를 생성하여 에디터에서 편집함.

| 필드명 | 타입 | 카테고리 | 설명 |
|--------|------|---------|------|
| `ItemID` | `FName` | Item Meta | 고유 식별자. `FD1InventoryItem::ItemID`와 1:1 매칭 |
| `ItemName` | `FText` | Item Meta | 게임 내 표시 이름 (한국어/영어 로컬라이징 가능) |
| `Description` | `FText` | Item Meta | 툴팁/상세 설명 |
| `ItemType` | `EItemType` | Item Meta | 대분류 |
| `EquipmentSlot` | `EEquipmentSlot` | Item Meta | 장비 부위 (Equipment일 때만 편집 가능) |
| `MaxStack` | `int32` | Item Meta | 최대 중첩 개수. 장비=1, 포션=20~99 등 |
| `Icon` | `UTexture2D*` | Item Visual | 인벤토리/툴팁 아이콘 |
| `EquipEffect` | `TSubclassOf<UGameplayEffect>` | Equipment | **장착 시** ASC에 적용될 GE |
| `UseEffect` | `TSubclassOf<UGameplayEffect>` | Consumable | **사용 시** ASC에 적용될 GE |
| `SellPrice` | `int32` | Trade | NPC 판매 가격 |
| `BuyPrice` | `int32` | Trade | NPC 구매 가격 (미구현) |

> **GE 기반 설계 이유:** GAS 프로젝트 특성상 `GameplayEffect`를 통해 Attribute를 수정하면, 장착/탈착 시 스탯이 **자동으로 원복**되고, 레벨 스케일링(`FScalableFloat`)도 자동 적용됨.

### 3.2 `UD1ItemRegistry` — 아이템 전체 목록

> 위치: `Source/D1/Inventory/D1ItemRegistry.h`  
> 상속: `UDataAsset`

에디터에서 **하나의** `.uasset`만 생성하여, **모든** `UD1ItemData` 인스턴스를 `ItemID`로 등록함.

```cpp
UCLASS()
class UD1ItemRegistry : public UDataAsset
{
    UPROPERTY(EditDefaultsOnly)
    TMap<FName, TObjectPtr<UD1ItemData>> Items;

    UD1ItemData* FindItemData(const FName& ItemID) const;
};
```

| 기능 | 설명 |
|------|------|
| `Items` | `ItemID` → `UD1ItemData*` 매핑. 에디터에서 Key-Value 형태로 등록 |
| `FindItemData` | `ItemID`로 아이템 검색. 없으면 `nullptr` 반환 |

### 3.3 `FD1InventoryItem` — 인벤토리 슬롯 (기존)

> 위치: `Source/D1/Inventory/D1InventoryTypes.h`

```cpp
USTRUCT(BlueprintType)
struct FD1InventoryItem
{
    FName ItemID;      // UD1ItemRegistry에서 룩업할 키
    int32 Count = 0;   // 현재 개수
    int32 SlotIndex;   // 인벤토리 슬롯 위치
};
```

> **런타임 룩업 체인:** `FD1InventoryItem.ItemID` → `UD1ItemRegistry::FindItemData()` → `UD1ItemData*` (이름, 아이콘, 효과 등 획득)

### 3.4 `UD1AbilitySystemLibrary::GetItemData()` — 헬퍼 함수

> 위치: `Source/D1/AbilitySystem/D1AbilitySystemLibrary.h`

```cpp
UFUNCTION(BlueprintCallable, Category = "D1AbilitySystemLibrary|ItemData")
static UD1ItemData* GetItemData(const UObject* WorldContextObject, const FName& ItemID);
```

내부적으로 `AD1GameModeBase::ItemRegistry`를 통해 `FindItemData`를 호출함.  
Blueprint나 C++ 어디서든 `WorldContext`만 있으면 아이템 메타데이터에 접근 가능.

---

## 4. 아이템 효과 설계 (GameplayEffect 연동)

### 4.1 장비 아이템 (`EquipEffect`)

장비를 **장착**하면 `ASC->ApplyGameplayEffectToSelf()`로 GE를 적용하고,  
**탈착**하면 `ASC->RemoveActiveGameplayEffect()`로 제거함.

**GE 설계 예시 (투구: 철 투구)**

| GE 속성 | 설정값 | 설명 |
|---------|--------|------|
| Duration | `Infinite` | 장착하는 동안 지속 |
| Modifiers | `+50 MaxHealth` | `FGameplayAttributeData` 직접 수정 |
| Stacking | `None` 또는 `AggregateByTarget` | 중복 장착 방지는 인벤토리/장비 슬롯 로직에서 처리 |

**구현 위치:** `UD1InventoryComponent` 또는 `UD1Hero` (장착/탈착 함수)

### 4.2 소비 아이템 (`UseEffect`)

아이템을 **사용**하면 `ServerUseItem` RPC → 서버에서 `UseItemInternal()` → `UseEffect` GE 적용.

**GE 설계 예시 (체력 포션)**

| GE 속성 | 설정값 | 설명 |
|---------|--------|------|
| Duration | `Instant` | 즉시 발동 |
| Modifiers | `+100 Health` (Clamp 필요) | `Health` 즉시 회복. `PreAttributeChange`에서 `MaxHealth` 초과 방지 |

**구현 위치:** `UD1InventoryComponent::UseItemInternal()`

### 4.3 GE가 아닌 효과 (특수 처리)

아래와 같은 효과는 GE로 표현하기 어려우므로, `UD1ItemData`에 추가 필드를 두거나 별도 로직으로 처리:

| 효과 유형 | 처리 방식 | 예시 |
|----------|----------|------|
| **골드/재화 획득** | `ServerUseItem` 내부에서 직접 처리 | 골드 주머니 |
| **스킬 초기화** | `UAbilitySystemComponent` 직접 호출 | 쿨타임 초기화 스크롤 |
| **텔레포트** | `APlayerController` 또는 `Character` 직접 조작 | 귀환석 |
| **랜덤 보상 상자** | 별도 확률 테이블 참조 | 랜덤 상자 |

> **확장 팁:** `UseEffect` 대신 `TSubclassOf<UD1ItemUseLogic>` 같은 커스텀 클래스 포인터를 추가하면, 더 유연한 소비 아이템 로직을 만들 수 있음 (Phase 2 고려).

---

## 5. 예시 아이템 정의

### 5.1 소비 아이템

| ItemID | ItemName | Type | MaxStack | UseEffect | SellPrice |
|--------|----------|------|----------|-----------|-----------|
| `Potion_Health_Small` | 작은 체력 포션 | Consumable | 20 | `GE_Potion_Health_Small` (+50 Health Instant) | 10 |
| `Potion_Mana_Small` | 작은 마나 포션 | Consumable | 20 | `GE_Potion_Mana_Small` (+30 Mana Instant) | 10 |
| `Scroll_Buff_Atk` | 공격력 강화 스크롤 | Consumable | 10 | `GE_Buff_AttackPower` (+20 AttackPower, 300초) | 50 |

### 5.2 장비 아이템 (워리어 초반 세트)

| ItemID | ItemName | Type | Slot | EquipEffect | SellPrice |
|--------|----------|------|------|-------------|-----------|
| `Sword_Iron` | 철 검 | Equipment | Weapon | `GE_Equip_Sword_Iron` (+15 AttackPower) | 100 |
| `Helmet_Iron` | 철 투구 | Equipment | Helmet | `GE_Equip_Helmet_Iron` (+50 MaxHealth, +2 HealthRegen) | 80 |
| `Armor_Iron` | 철 갑옷 | Equipment | Armor | `GE_Equip_Armor_Iron` (+30 Armor, +30 MaxHealth) | 120 |
| `Gloves_Leather` | 가죽 장갑 | Equipment | Gloves | `GE_Equip_Gloves_Leather` (+5 AttackPower, +2% CritChance) | 60 |
| `Boots_Leather` | 가죽 신발 | Equipment | Boots | `GE_Equip_Boots_Leather` (+20 MaxHealth, +1% Dodge) | 60 |
| `Necklace_Mana` | 마나 목걸이 | Equipment | Necklace | `GE_Equip_Necklace_Mana` (+30 MaxMana, +1 ManaRegen) | 70 |
| `Ring_Strength` | 힘의 반지 | Equipment | Ring | `GE_Equip_Ring_Strength` (+5 Strength) | 90 |

> **Phase 1 (언리얼 내부):** 위 아이템들을 에디터에서 `UD1ItemData` 인스턴스로 생성하고, `UD1ItemRegistry`에 등록함.  
> **Phase 2 (웹서버 연동):** `ItemID`, `Count`만 DB에 저장. 메타데이터는 서버도 `UD1ItemRegistry`를 통해 참조하거나, 웹서버에 동일한 Item Meta 테이블을 별도로 둠.

---

## 6. 인벤토리 연동 흐름

### 6.1 아이템 획득

```
[몬스터 처치 / 상자 오픈]
    ↓
[Dedicated Server] ServerGiveReward()
    ↓
UD1InventoryComponent::AddItem("Sword_Iron", 1)
    ↓
서버: InventorySlots 배열 갱신 (Replicated)
    ↓
[Client] OnRep_Inventory() → UI 갱신
    ↓
WBP_InventorySlot: ItemID → GetItemData() → 아이콘/이름 표시
```

### 6.2 아이템 사용 (소비)

```
[Client] WBP_InventorySlot: 우클릭 "사용"
    ↓
UD1InventoryComponent::ServerUseItem(SlotIndex)
    ↓
[Dedicated Server] UseItemInternal()
    ↓
1. GetItemData(ItemID) 로 메타데이터 확인
2. ItemType == Consumable 확인
3. UseEffect GE를 ASC에 Apply
4. InventorySlots[SlotIndex].Count-- (0이면 슬롯 비우기)
5. OnInventoryChanged Broadcast → 클라이언트 동기화
```

### 6.3 장비 장착 (추후 확장)

```
[Client] WBP_InventorySlot: 더블클릭 "장착"
    ↓
UD1InventoryComponent::ServerEquipItem(SlotIndex) (미구현)
    ↓
[Dedicated Server]
    ↓
1. GetItemData(ItemID) → EquipmentSlot 확인
2. 현재 해당 Slot에 장착된 아이템이 있으면 탈착 (EquipEffect Remove)
3. 새 아이템 장착 (EquipEffect Apply)
4. Inventory에서 해당 슬롯의 Count 감소 또는 별도 EquipSlots 배열로 이동
```

> **참고:** 현재 `UD1InventoryComponent`에는 `ServerEquipItem`이 없음. Phase 1 이후 또는 인벤토리 기본 기능 완료 후 추가 예정.

---

## 7. 웹서버 연동 시 고려사항

### 7.1 영속 데이터 (DB 저장)

| 테이블 | 필드 | 설명 |
|--------|------|------|
| **Inventory** | `player_id`, `slot_index`, `item_id`, `count` | 인벤토리 슬롯별 저장 |
| **EquippedItems** (추후) | `player_id`, `equipment_slot`, `item_id` | 현재 장착 중인 장비 |

> **메타데이터는 저장하지 않음.** `item_id`만 저장하고, 룩업은 `UD1ItemRegistry` (언리얼) 또는 웹서버의 `Item_Meta` 정적 테이블로 수행.

### 7.2 서버-클라이언트 검증 원칙

- 클라이언트는 **절대** 아이템 사용/장착/이동을 클라이언트에서만 처리하지 않음.
- 모든 변경은 `ServerXXX` RPC를 통해 Dedicated Server로 전송.
- Dedicated Server는 `UD1ItemRegistry`를 통해 `ItemID`의 존재 여부, `MaxStack`, `ItemType` 등을 검증한 후 처리.
- Phase 2에서 Dedicated Server는 처리 후 웹서버에 `Inventory` DB를 갱신 요청.

### 7.3 Item Meta 동기화

웹서버에서도 아이템 이름, 설명, 가격 등을 표시해야 할 수 있음 (로비 상점, 웹 인벤토리 등).  
이 경우 웹서버에도 아래와 같은 **정적 테이블**을 두는 것이 좋음:

```sql
CREATE TABLE item_meta (
    item_id VARCHAR(64) PRIMARY KEY,
    item_name VARCHAR(128),
    item_type ENUM('Consumable','Equipment','Material','Quest','Etc'),
    equipment_slot VARCHAR(32), -- NULL 가능
    max_stack INT,
    sell_price INT,
    buy_price INT,
    description TEXT
);
```

> **동기화 전략:** 언리얼 `UD1ItemRegistry`를 기준으로 CSV/JSON 익스포트 → 웹서버 `item_meta` 테이블 Import.  
> 게임 배포 시마다 한 번씩 동기화하면 충분함 (런타임 변경 없음).

---

## 8. TODO / 확장 아이디어

| 우선순위 | 항목 | 설명 | 예상 시기 |
|---------|------|------|----------|
| **필수** | `ServerEquipItem` / `ServerUnequipItem` | 장비 장착/탈착 RPC 및 `EquipEffect` 적용/제거 로직 | Phase 1 후반 |
| **필수** | `UD1ItemData` 에디터 인스턴스 생성 | 예시 아이템 7종 이상 실제 `.uasset`으로 제작 | Phase 1 |
| **권장** | 아이템 등급/희귀도 (`EItemRarity`) | Common, Rare, Epic, Legendary 등. UI 테두리 색상/드롭률에 영향 | Phase 1~2 |
| **권장** | 인벤토리 슬롯 확장 | `MaxSlots`를 기본 20 → 골드/아이템으로 확장 가능하도록 | Phase 2 |
| **확장** | `InstanceData` (가변 아이템 속성) | 강화 레벨, 내구도, 접두사/접미사 등. `FInstancedStruct` 활용 | Phase 2 이후 |
| **확장** | 아이템 세트 효과 | 동일 세트 장비 2/4/6개 착용 시 추가 보너스 GE 적용 | Phase 2 이후 |
| **확장** | `UD1ItemUseLogic` 커스텀 클래스 | 소비 아이템의 특수 효과(텔레포트, 랜덤 상자 등)를 GE 외 별도 로직으로 분리 | Phase 2 이후 |
| **확장** | 장비 제작/강화 시스템 | `Material` 타입 아이템을 소모하여 장비 능력치 상승 | Phase 2 이후 |

---

*문서 작성일: 2026-05-03*  
*기준 코드:* `D1InventoryTypes.h`, `D1ItemData.h`, `D1ItemRegistry.h`, `D1AbilitySystemLibrary.h/cpp`
