# 작업 로그: 2026-05-03 (Item System)

## 작업 내용

### 1. 아이템 분류 및 장비 부위 Enum 추가
- **파일:** `Source/D1/Inventory/D1InventoryTypes.h`
- **추가 내용:**
  - `EItemType`: None, Consumable, Equipment, Material, Quest, Etc
  - `EEquipmentSlot`: None, Weapon, Helmet, Armor, Gloves, Boots, Necklace, Ring

### 2. 아이템 메타데이터 클래스 작성
- **파일:** `Source/D1/Inventory/D1ItemData.h`
- **내용:** `UDataAsset` 상속, 개별 아이템 메타데이터 필드 추가
  - `ItemID`, `ItemName`, `Description`, `ItemType`, `EquipmentSlot`, `MaxStack`, `Icon`
  - `EquipEffect` (장비용 GE), `UseEffect` (소비용 GE)
  - `SellPrice`, `BuyPrice`

### 3. 아이템 레지스트리 클래스 신규 생성
- **파일:** `Source/D1/Inventory/D1ItemRegistry.h`, `.cpp`
- **내용:** `UDataAsset` 상속, `TMap<FName, TObjectPtr<UD1ItemData>> Items`로 모든 아이템 등록
  - `FindItemData(const FName& ItemID)` 함수 제공

### 4. GameMode에 ItemRegistry 연결
- **파일:** `Source/D1/Game/D1GameModeBase.h`
- **내용:** `TObjectPtr<UD1ItemRegistry> ItemRegistry;` 필드 추가

### 5. AbilitySystemLibrary에 아이템 룩업 헬퍼 추가
- **파일:** `Source/D1/AbilitySystem/D1AbilitySystemLibrary.h`, `.cpp`
- **내용:** `static UD1ItemData* GetItemData(const UObject* WorldContextObject, const FName& ItemID);` 추가
  - 난부적으로 `AD1GameModeBase->ItemRegistry->FindItemData()` 호출

### 6. 아이템 기획서 작성
- **파일:** `ItemDataReference.md`
- **내용:**
  - 아이템 분류 및 장비 부위 상세 정의
  - C++ 데이터 구조 상세 설명
  - GameplayEffect 연동 설계 (장착/사용/탈착)
  - 예시 아이템 정의 (소비 3종, 장비 7종)
  - 인벤토리 연동 흐름도
  - 웹서버 연동 시 DB 스키마 및 동기화 전략
  - TODO / 확장 아이디어

## 비고
- 현재 `ServerEquipItem` / `ServerUnequipItem`은 미구현. Phase 1 인벤토리 기본 기능 완료 후 추가 예정.
- `D1ItemData` 에디터 인스턴스(.uasset) 생성은 다음 작업으로 진행 필요.
