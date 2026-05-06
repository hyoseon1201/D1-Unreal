# D1 프로젝트 게임 데이터 레퍼런스

> **목적:** 언리얼 엔진의 C++ 소스코드 기반 Attribute, Skill, CharacterClass, Inventory 등의 데이터 구조 및 태그 체계를 정리.  
> **대상 독자:** 웹서버(Spring Boot) 개발자 및 클라이언트/서버 개발자.  
> **기준 시점:** 2026-05-03 (Context.md 기준 Phase 1: 인벤토리 진행 중)

---

## 목차

1. [Gameplay Tags 전체 레지스트리](#1-gameplay-tags-전체-레지스트리)
2. [Attribute (능력치) 시스템](#2-attribute-능력치-시스템)
3. [Ability / Skill 시스템](#3-ability--skill-시스템)
4. [Character Class 시스템](#4-character-class-시스템)
5. [Level-up 시스템](#5-level-up-시스템)
6. [Inventory 시스템](#6-inventory-시스템)
7. [Damage & Combat 구조](#7-damage--combat-구조)
8. [DataAsset 종합 목록](#8-dataasset-종합-목록)

---

## 1. Gameplay Tags 전체 레지스트리

> `FD1GameplayTags::InitializeNativeGameplayTags()`에서 `UGameplayTagsManager`에 등록됨.  
> 웹서버에서도 동일한 태그 문자열을 사용해 상태를 식별하거나 필터링할 수 있음.

### 1.1 Attributes (능력치)

| 분류 | 태그 문자열 | 설명 |
|------|-----------|------|
| **Vital** | `Attributes.Vital.Health` | 현재 체력 |
| **Vital** | `Attributes.Vital.Mana` | 현재 마나 |
| **Primary** | `Attributes.Primary.Strength` | 물리 피해 증가 |
| **Primary** | `Attributes.Primary.Intelligence` | 마법 피해 증가 |
| **Primary** | `Attributes.Primary.Dexterity` | 방어 관통 및 치명타 피해 증가 |
| **Primary** | `Attributes.Primary.Luck` | 치명타 확률 및 방어 관통 증가 |
| **Secondary** | `Attributes.Secondary.AttackPower` | 계산된 공격력 |
| **Secondary** | `Attributes.Secondary.Armor` | 받는 피해 감소, 블록 확률 향상 |
| **Secondary** | `Attributes.Secondary.ArmorPenetration` | 대상 방어율 일정 비율 무시 |
| **Secondary** | `Attributes.Secondary.CriticalHitChance` | 치명타 확률 (2배+볶너스) |
| **Secondary** | `Attributes.Secondary.CriticalHitDamage` | 치명타 발생 시 추가 피해 |
| **Secondary** | `Attributes.Secondary.MaxHealth` | 최대 체력 |
| **Secondary** | `Attributes.Secondary.MaxMana` | 최대 마나 |
| **Secondary** | `Attributes.Secondary.HealthRegeneration` | 초당 체력 재생량 |
| **Secondary** | `Attributes.Secondary.ManaRegeneration` | 초당 마나 재생량 |
| **Meta** | `Attributes.Meta.IncomingXP` | 획득 예정 경험치 (런타임 임시) |

### 1.2 Input (입력)

| 태그 문자열 | 설명 |
|-----------|------|
| `InputTag.LMB` | 마우스 좌클릭 |
| `InputTag.RMB` | 마우스 우클릭 |
| `InputTag.Q` | Q 키 |
| `InputTag.W` | W 키 |
| `InputTag.E` | E 키 |
| `InputTag.R` | R 키 |
| `InputTag.1` ~ `InputTag.4` | 숫자 키 1~4 |

### 1.3 Montage (애니메이션 몽타주)

| 태그 문자열 | 설명 |
|-----------|------|
| `Montage.GroundSword` | GroundSword 몽타주 |
| `Montage.Attack.Weapon` | 무기 공격 몽타주 |
| `Montage.Attack.RightHand` | 오른손 공격 몽타주 |
| `Montage.Attack.LeftHand` | 왼손 공격 몽타주 |

### 1.4 Combat Socket (전투 소켓)

| 태그 문자열 | 설명 |
|-----------|------|
| `CombatSocket.Weapon` | 무기 소켓 |
| `CombatSocket.RightHand` | 오른손 소켓 |
| `CombatSocket.LeftHand` | 왼손 소켓 |

### 1.5 Player Block (입력 차단)

| 태그 문자열 | 설명 |
|-----------|------|
| `Player.Block.InputPressed` | Pressed 입력 차단 |
| `Player.Block.Held` | Held 입력 차단 |
| `Player.Block.Released` | Released 입력 차단 |

### 1.6 Abilities (스킬)

| 분류 | 태그 문자열 | 설명 |
|------|-----------|------|
| **Active** | `Abilities.Attack` | 기본 공격 |
| **Active** | `Abilities.Physical.GroundSword` | 워리어 | GroundSword |
| **Active** | `Abilities.Physical.WhirlwindCharge` | 워리어 | WhirlwindCharge |
| **Passive** | `Abilities.TestPassive` | 워리어 | TestPassive |
| **Reaction** | `Abilities.HitReact` | 피격 반응 |
| **Status** | `Abilities.Status.Locked` | 잠김 |
| **Status** | `Abilities.Status.Eligible` | 해금 가능 |
| **Status** | `Abilities.Status.Unlocked` | 해금됨 |
| **Status** | `Abilities.Status.Equipped` | 장착됨 |
| **Type** | `Abilities.Type.Active` | 액티브 스킬 |
| **Type** | `Abilities.Type.Passive` | 패시브 스킬 |
| **Type** | `Abilities.Type.None` | 미분류 |

### 1.7 Class (직업)

| 태그 문자열 | 설명 |
|-----------|------|
| `Class.Warrior` | 워리어 클래스 |

### 1.8 Cooldown (쿨타임)

| 태그 문자열 | 설명 |
|-----------|------|
| `Cooldown.Physical.GroundSword` | GroundSword 쿨타임 |
| `Cooldown.Physical.WhirlwindCharge` | WhirlwindCharge 쿨타임 |

### 1.9 Damage (피해)

| 태그 문자열 | 설명 |
|-----------|------|
| `Damage` | 피해 (상위) |
| `Damage.Physical` | 물리 피해 |

### 1.10 Effects (효과)

| 태그 문자열 | 설명 |
|-----------|------|
| `Effects.HitReact` | 피격 반응 효과 |

---

## 2. Attribute (능력치) 시스템

### 2.1 `UD1AttributeSet` (`UAttributeSet` 상속)

> 소스: `Source/D1/AbilitySystem/D1AttributeSet.h`

모든 능력치는 `FGameplayAttributeData`로 관리되며, `ReplicatedUsing`으로 클라이언트에 동기화됨.  
웹서버 연동 시에는 **Primary + Secondary** 중심으로 영속화하면 됨 (Vital은 런타임 상태).

#### Vital Attributes (생존)

| 필드명 | 카테고리 | 동기화 | 설명 |
|--------|---------|--------|------|
| `Health` | Vital | `OnRep_Health` | 현재 체력 |
| `Mana` | Vital | `OnRep_Mana` | 현재 마나 |

#### Primary Attributes (1차 능력치)

| 필드명 | 카테고리 | 동기화 | 설명 |
|--------|---------|--------|------|
| `Strength` | Primary | `OnRep_Strength` | 물리 피해 증가 |
| `Intelligence` | Primary | `OnRep_Intelligence` | 마법 피해 증가 |
| `Dexterity` | Primary | `OnRep_Dexterity` | 방어 관통 및 치명타 피해 증가 |
| `Luck` | Primary | `OnRep_Luck` | 치명타 확률 및 방어 관통 증가 |

#### Secondary Attributes (2차 능력치)

| 필드명 | 카테고리 | 동기화 | 설명 | 계산 근거 |
|--------|---------|--------|------|----------|
| `AttackPower` | Secondary | `OnRep_AttackPower` | 공격력 | `MMC_AttackPower` (Primary 기반) |
| `Armor` | Secondary | `OnRep_Armor` | 방어력 | `MMC_Armor` |
| `ArmorPenetration` | Secondary | `OnRep_ArmorPenetration` | 방어 관통 | `MMC_ArmorPenetration` |
| `CriticalHitChance` | Secondary | `OnRep_CriticalHitChance` | 치명타 확률 | `MMC_CriticalHitChance` |
| `CriticalHitDamage` | Secondary | `OnRep_CriticalHitDamage` | 치명타 피해 | `MMC_CriticalHitDamage` |
| `MaxHealth` | Secondary | `OnRep_MaxHealth` | 최대 체력 | `MMC_MaxHealth` |
| `MaxMana` | Secondary | `OnRep_MaxMana` | 최대 마나 | `MMC_MaxMana` |
| `HealthRegeneration` | Secondary | `OnRep_HealthRegeneration` | 체력 재생 | `MMC_HealthRegeneration` |
| `ManaRegeneration` | Secondary | `OnRep_ManaRegeneration` | 마나 재생 | `MMC_ManaRegeneration` |

> **MMC (Mod Magnitude Calculation):** `Source/D1/AbilitySystem/MMC/` 폴더에 각각의 계산 로직이 존재함.

#### Meta Attributes (메타, 임시)

| 필드명 | 카테고리 | 동기화 | 설명 |
|--------|---------|--------|------|
| `IncomingDamage` | Meta | 없음 | 받을 피해량 (GE 실행 중 임시) |
| `IncomingXP` | Meta | 없음 | 받을 경험치 (GE 실행 중 임시) |

### 2.2 `UD1AttributeInfo` (`UDataAsset`)

> 소스: `Source/D1/AbilitySystem/Data/D1AttributeInfo.h`

UI(Attribute Menu 등)에서 능력치의 **이름, 설명, 현재값**을 표시하기 위한 DataAsset.

```cpp
USTRUCT(BlueprintType)
struct FD1AttributeTagInfo
{
    FGameplayTag AttributeTag;      // 예: Attributes.Primary.Strength
    FText AttributeName;            // 표시 이름 (에디터에서 설정)
    FText AttributeDescription;     // 설명 (에디터에서 설정)
    float AttributeValue = 0.f;     // 런타임에 채워지는 현재값
};

UCLASS()
class UD1AttributeInfo : public UDataAsset
{
    TArray<FD1AttributeTagInfo> AttributeInformation;
    FD1AttributeTagInfo FindAttributeInfoForTag(const FGameplayTag& AttributeTag, ...) const;
};
```

**웹서버 참고:** `AttributeTag` 문자열과 매핑되는 스탯 ID 체계를 설계하면 UI-서버 간 일관성 유지에 유리함.

---

## 3. Ability / Skill 시스템

### 3.1 클래스 계층 구조

```
UGameplayAbility (엔진 기본)
└── UD1GameplayAbility
    └── UD1DamageGameplayAbility
        ├── UD1ProjectileSpell (투사체 마법)
        └── UD1MeleeAttack (근접 공격)
```

#### `UD1GameplayAbility`

| 필드명 | 타입 | 설명 |
|--------|------|------|
| `StartupInputTag` | `FGameplayTag` | 기본 입력 태그 (예: `InputTag.LMB`) |

#### `UD1DamageGameplayAbility`

| 필드명 | 타입 | 설명 |
|--------|------|------|
| `DamageEffectClass` | `TSubclassOf<UGameplayEffect>` | 피해를 주는 GameplayEffect 클래스 |
| `DamageTypes` | `TMap<FGameplayTag, FScalableFloat>` | 태그별 데미지 계수 (예: `Damage.Physical` → 50.0) |

> `CauseDamage(AActor* TargetActor)` 함수로 대상에게 피해를 입힘.

#### `UD1ProjectileSpell`

| 필드명 | 타입 | 설명 |
|--------|------|------|
| `ProjectileClass` | `TSubclassOf<AD1Projectile>` | 발사할 투사체 BP 클래스 |

> `SpawnProjectile(const FVector& TargetLocation, const FGameplayTag& SocketTag)`로 투사체 생성.

#### `UD1MeleeAttack`

> 별도 필드 없음. `UD1DamageGameplayAbility`의 기본 근접 공격 구현체.

### 3.2 `UD1AbilityInfo` (`UDataAsset`) — 스킬 메타데이터

> 소스: `Source/D1/AbilitySystem/Data/D1AbilityInfo.h`

스킬 목록, 아이콘, 쿨타임, 레벨 제한 등을 관리하는 DataAsset.  
**Skill Menu, HUD 아이콘 표시, 쿨타임 UI** 등에서 참조함.

```cpp
USTRUCT(BlueprintType)
struct FD1AbilityTagInfo
{
    FGameplayTag AbilityTag;            // 스킬 고유 태그 (예: Abilities.Physical.GroundSword)
    FGameplayTag SkillTypeTag;          // 스킬 타입 태그
    FGameplayTag ClassTag;              // 사용 가능 클래스 태그 (예: Class.Warrior)
    FGameplayTag InputTag;              // 현재 할당된 입력 태그 (런타임)
    FGameplayTag StatusTag;             // 현재 상태 태그 (Locked/Eligible/Unlocked/Equipped)
    FGameplayTag CooldownTag;           // 쿨타임 태그
    TObjectPtr<const UTexture2D> Icon;  // 아이콘 텍스처
    int32 SlotIndex;                    // UI 슬롯 인덱스
    int32 LevelRequirement = 1;         // 요구 레벨
    TSubclassOf<UGameplayAbility> Ability; // 실제 GameplayAbility 클래스
    FText AbilityName;                  // 스킬 이름
    FText AbilityDescription;           // 스킬 설명
    int32 Level = 0;                    // 현재 스킬 레벨 (런타임)
};

UCLASS()
class UD1AbilityInfo : public UDataAsset
{
    TArray<FD1AbilityTagInfo> AbilityInformation;
    FD1AbilityTagInfo FindAbilityTagInforTag(...) const;
};
```

**웹서버 참고:** 웹서버에서 스킬 해금 상태, 레벨, 장착 여부를 저장할 때 `AbilityTag` 문자열을 PK 또는 외래키로 사용하면 언리얼과 직접 매핑 가능.

---

## 4. Character Class 시스템

### 4.1 `ECharacterClass` (Enum)

> 소스: `Source/D1/AbilitySystem/Data/D1CharacterClassInfo.h`

현재 정의된 클래스:

| 값 | 설명 |
|---|------|
| `Goblin_Melee` | 고블린 근접 (Enemy) |
| `Goblin_Ranger` | 고블린 원거리 (Enemy) |

> 플레이어 클래스(`Class.Warrior`)는 GameplayTag로 관리되며, 이 Enum은 주로 **Enemy/NPC** 클래스에 사용됨.

### 4.2 `FCharacterClassDefaultInfo`

| 필드명 | 타입 | 설명 |
|--------|------|------|
| `PrimaryAttribute` | `TSubclassOf<UGameplayEffect>` | 클래스별 1차 능력치 초기화 GE |
| `StartupAbilities` | `TArray<TSubclassOf<UGameplayAbility>>` | 클래스 기본 부여 능력 |
| `XPReward` | `FScalableFloat` | 처치 시 주는 경험치 |

### 4.3 `UD1CharacterClassInfo` (`UDataAsset`)

| 필드명 | 타입 | 설명 |
|--------|------|------|
| `CharacterClassInformation` | `TMap<ECharacterClass, FCharacterClassDefaultInfo>` | 클래스별 기본 정보 맵 |
| `SecondaryAttributes` | `TSubclassOf<UGameplayEffect>` | 모든 클래스 공통 2차 능력치 GE |
| `VitalAttributes` | `TSubclassOf<UGameplayEffect>` | 모든 클래스 공통 생존 능력치 GE |
| `CommonAbilities` | `TArray<TSubclassOf<UGameplayAbility>>` | 모든 클래스 공통 능력 (예: HitReact) |
| `DamageCalculationCoefficients` | `TObjectPtr<UCurveTable>` | 데미지 계산 커브 테이블 |

**웹서버 참고:** 플레이어의 클래스(워리어 등)는 태그로, 몬스터의 클래스는 Enum으로 관리되므로 DB 스키마 설계 시 두 체계를 구분하거나 통일해야 함.

---

## 5. Level-up 시스템

### 5.1 `UD1LevelupInfo` (`UDataAsset`)

> 소스: `Source/D1/AbilitySystem/Data/D1LevelupInfo.h`

```cpp
USTRUCT(BlueprintType)
struct FD1LevelupTagInfo
{
    int32 LevelupRequirement = 0;   // 해당 레벨로 올라가기 위해 필요한 **누적 경험치**
    int32 AttributePointAward = 1;  // 레벨업 시 지급되는 스탯 포인트
    int32 SpellPointAward = 1;      // 레벨업 시 지급되는 스킬 포인트
};

UCLASS()
class UD1LevelupInfo : public UDataAsset
{
    TArray<FD1LevelupTagInfo> LevelupInformation;
    int32 FindLevelForXP(int32 XP) const;  // 현재 XP로 레벨 계산
};
```

**인덱스 규칙:** `LevelupInformation[0]` = Level 1 → Level 2 요구치.  
즉, 배열 인덱스 + 1이 **목표 레벨**이 됨.

**웹서버 참고:** 영속 데이터로 `CurrentLevel`, `CurrentXP`, `AttributePoints`, `SpellPoints`를 저장해야 하며,  
로그인 시 `LevelupInfo` 데이터를 기준으로 `CurrentXP` → `CurrentLevel` 검증 및 동기화 필요.

---

## 6. Inventory 시스템

### 6.1 `FD1InventoryItem` (구조체)

> 소스: `Source/D1/Inventory/D1InventoryTypes.h`

```cpp
USTRUCT(BlueprintType)
struct FD1InventoryItem
{
    FName ItemID;       // 아이템 고유 ID (DataAsset 이름 또는 테이블 키)
    int32 Count = 0;    // 개수 (스택 가능 아이템용)
    int32 SlotIndex = -1; // 인벤토리 슬롯 위치
};
```

> **주의:** 현재 구조에는 아이템의 `InstanceData` (강화값, 내구도 등) 필드가 없음.  
> 나중에 `FInstancedStruct` 또는 커스텀 구조체로 확장 가능.

### 6.2 `UD1InventoryComponent` (`UActorComponent`)

> 소스: `Source/D1/Inventory/D1InventoryComponent.h`  
> 부착 대상: `PlayerState` (멀티플레이어 기준)

#### 서버 RPC (클라이언트 → 서버)

| 함수명 | 파라미터 | 설명 |
|--------|---------|------|
| `ServerUseItem` | `int32 SlotIndex` | 슬롯의 아이템 사용 요청 |
| `ServerMoveItem` | `int32 FromIndex, int32 ToIndex` | 슬롯 이동 요청 |
| `ServerDiscardItem` | `int32 SlotIndex` | 아이템 버리기 요청 |

#### 서버 전용 함수

| 함수명 | 파라미터 | 반환 | 설명 |
|--------|---------|------|------|
| `AddItem` | `const FName& ItemID, int32 Count` | `bool` | 아이템 추가 (보상/획득) |
| `RemoveItem` | `int32 SlotIndex, int32 Count` | `bool` | 아이템 제거 |
| `GetInventorySlots` | - | `TArray<FD1InventoryItem>&` | 현재 인벤토리 읽기 |

#### 프로퍼티

| 필드명 | 타입 | 설명 |
|--------|------|------|
| `MaxSlots` | `int32` (기본값 20) | 최대 슬롯 수 |
| `InventorySlots` | `TArray<FD1InventoryItem>` (Replicated) | 실제 인벤토리 배열 |
| `OnInventoryChanged` | `FOnInventoryChanged` (Multicast Delegate) | 변경 시 UI 알림 |

**웹서버 참고:** Phase 2에서 Dedicated Server가 웹서버에 `InventorySlots` 배열을 동기화해야 함.  
서버는 `AddItem`/`RemoveItem` 시 웹서버에 HTTP 요청하여 DB 갱신 → 클라이언트는 여전히 RPC만 사용.

---

## 7. Damage & Combat 구조

### 7.1 데미지 계산 흐름

1. **공격자**가 `UD1DamageGameplayAbility` 활성화
2. `DamageTypes` 맵에서 `FGameplayTag`(예: `Damage.Physical`) → `FScalableFloat` 조회
3. `CauseDamage(TargetActor)` 호출 → `DamageEffectClass` GE를 대상에 적용
4. `UD1AttributeSet::PostGameplayEffectExecute`에서 `IncomingDamage` 처리
5. `ExecCalc_Damage` (Execution Calculation)에서 최종 데미지 산출  
   - `Armor`, `ArmorPenetration`, `CriticalHitChance`, `CriticalHitDamage` 등 참조
6. 처치 시 `XPReward`를 `IncomingXP`로 전달 → 경험치 처리

### 7.2 관련 DataAsset

| 클래스 | 역할 |
|--------|------|
| `UD1AbilitySystemConfig` | 전역 데미지 계산 커브 테이블 (`DamageCalculationCoefficients`) 참조 |
| `UD1CharacterClassInfo` | 클래스별 `XPReward`, `DamageCalculationCoefficients` 참조 |

---

## 8. DataAsset 종합 목록

> 아래 DataAsset들은 에디터에서 `.uasset` 파일로 존재하며, C++ 클래스는 스키마(필드 정의)만 담당함.  
> **실제 데이터(수치, 아이콘, 이름 등)**는 에디터에서 편집된 값을 참조해야 함.

| C++ 클래스 | 파일 위치 예시 | 보관 데이터 | 웹서버 연동 시 필요 여부 |
|-----------|--------------|-----------|---------------------|
| `UD1AttributeInfo` | `Content/Blueprints/Data/AttributeInfo.uasset` | 능력치 이름/설명/태그 매핑 | 중간 (스탯 UI 레퍼런스용) |
| `UD1AbilityInfo` | `Content/Blueprints/Data/AbilityInfo.uasset` | 스킬 메타데이터 (이름, 아이콘, 레벨제한, GE/Ability 클래스 참조) | **높음** (스킬 해금/장착 정보 연동) |
| `UD1CharacterClassInfo` | `Content/Blueprints/Data/CharacterClassInfo.uasset` | 클래스별 초기 능력치 GE, 기본 스킬, XP 보상, 데미지 커브 | 중간 (몬스터 데이터 정의용) |
| `UD1LevelupInfo` | `Content/Blueprints/Data/LevelupInfo.uasset` | 레벨업 별 필요 XP, 지급 포인트 | **높음** (레벨/XP 검증에 필수) |
| `UD1AbilitySystemConfig` | `Content/Blueprints/Data/AbilitySystemConfig.uasset` | 전역 데미지 계산 커브 테이블 | 낮음 (서버도 동일 테이블 필요할 수 있음) |
| `UD1ItemData` | (Context.md 언급) | 아이템 메타데이터 (이름, 아이콘, 설명, 최대중첩, 효과) | **높음** (인벤토리/상점 연동) |

---

## 부록: 웹서버 → 언리얼 데이터 흐름 (Phase 2 예상)

```
[Client]
   ↕ RPC (ServerUseItem, ServerMoveItem, ServerEquipSkill...)
[Dedicated Server (UE5)]
   ↕ HTTP/JSON (REST API)
[Spring Boot Web Server]
   ↕ SQL/JPA
[Database (MySQL/PostgreSQL)]
```

### 영속화 대상 데이터 (DB 테이블 후보)

| 도메인 | 주요 필드 예시 |
|--------|--------------|
| **User / Player** | UUID, Nickname, ClassTag, CurrentLevel, CurrentXP |
| **Attributes** | Strength, Intelligence, Dexterity, Luck (Primary만 저장, Secondary는 서버에서 재계산) |
| **Skills** | AbilityTag, Level, IsUnlocked, IsEquipped, SlotIndex |
| **Inventory** | SlotIndex, ItemID, Count (InstanceData 확장 시 JSON 컬럼 고려) |
| **Levelup Config** | Level, RequiredXP, AttributePointAward, SpellPointAward (정적 테이블) |
| **Item Meta** | ItemID, Name, Description, MaxStack, EffectType, EffectValue (정적 테이블) |

### 동기화 타이밍 권장

- **Load:** 플레이어 접속 시 Web Server → Dedicated Server → PlayerState/InventoryComponent/GAS AttributeSet 초기화
- **Save:** 중요 이벤트(레벨업, 인벤토리 변경, 스킬 해금) 발생 시 즉시 저장, 세션 종료 시 최종 저장
- **Runtime:** HP, Mana, Cooldown 등은 GAS Replication으로 처리, 웹서버와 실시간 동기화 불필요

---

*문서 생성일: 2026-05-03*  
*기준 브랜치/커밋: Context.md Phase 1 (인벤토리 진행 중)*
