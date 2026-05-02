# 프로젝트 개요: D1

## 1. 핵심 기술 스택
- **엔진:** Unreal Engine 5 (UE5)
- **언어:** C++ (주력), Blueprints (필요 시)
- **프레임워크:** Gameplay Ability System (GAS)
- **네트워킹:** 전용 서버 (Dedicated Server) 기반 멀티플레이어
- **입력:** Enhanced Input System

## 2. 프로젝트 목표
- 탑다운 뷰의 몰입감 있는 멀티플레이 액션 RPG 경험 제공.
- GAS를 활용한 체계적이고 확장 가능한 스킬/상태 시스템 구축.

## 3. 현재 진행 상황

| # | 작업 항목 | 상태 |
|---|---------|------|
| 1 | 플레이어 캐릭터 기본 클래스 구현 | ✅ 완료 |
| 2 | 멀티플레이어 연동 및 동기화 | ✅ 완료 |
| 3 | 기본적인 캐릭터/능력치 클래스 구조 | ✅ 완료 |
| 4 | Attribute Menu, Skill Menu 위젯 연결 | ✅ 완료 |
| 5 | HUD 스킬 아이콘 및 쿨타임 표시 | ✅ 완료 |
| 6 | 인벤토리 시스템 (언리얼 납부) | 🔄 진행 중 |
| 7 | 웹서버 연동 (로비/인벤토리/스탯 영속화) | ⏳ 대기 |
| 8 | 로비/던전 분리 (Gameplay Mode) | ⏳ 대기 |

### 5번 상세 (완료)
- **구조:** `D1OverlayWidgetController` → `OnCooldownTagChangedDelegate` Broadcast → `WBP_SkillSlot` 바인딩
- **C++ 수정:**
  - `FOnCooldownSignature`를 `ThreeParams` → `FourParams`로 변경 (`Duration` 추가).
  - `OnCooldownTagChanged`: Cooldown GE가 여러 개일 경우 가장 긴 `TimeRemaining`을 찾도록 순회.
- **WBP 작업 (사용자 직접 수행):**
  - `AbilityInfoDelegate` 수신 시 `CurrentAbilityTag` 저장
  - `OnCooldownTagChangedDelegate` 수신 → `CurrentAbilityTag` 필터링 → `CooldownPercent` 확인
  - `Percent > 0`: `ImageCooldown` Visible + `CooldownDynamicMat.SetScalarParameterValue("Progress", Percent)` + `Set Timer by Event(0.03s, Looping)` → `UpdateCooldown` Custom Event
  - `Percent == 0`: `Clear and Invalidate Timer` + `Progress = 0` + `ImageCooldown` Hidden
  - `UpdateCooldown`: `ElapsedTime += 0.03` → `1.0 - (ElapsedTime / CooldownDuration)` → 머터리얼 Progress 갱신
- **머터리얼 수정:** `M_Cooldown`의 `VectorToRadialValue` 입력 X축에 `Multiply(-1, 1)` 적용하여 회전 방향 반시계 → 시계로 변경

### 6번 상세 (현재 집중 작업: 인벤토리 시스템)
- **Phase 1 (언리얼 납부, 현재):** 웹서버 없이 언리얼 낶에서만 동작하는 인벤토리 시스템 구축
  - `UD1ItemData` (UPrimaryDataAsset): 아이템 메타데이터 (이름, 아이콘, 설명, 최대중첩, 효과 등)
  - `FD1InventoryItem` (USTRUCT): 인벤토리 슬롯 (ItemID, Count, InstanceData)
  - `UD1InventoryComponent` (UActorComponent): PlayerState에 부착, 실제 인벤토리 로직 담당
    - `ServerUseItem()`, `ServerMoveItem()`, `ServerDiscardItem()` (RPC)
    - `AddItem()`, `RemoveItem()` (서버 전용)
  - `WBP_Inventory` / `WBP_InventorySlot` (Blueprint): UI (클이언트)
  - 데이터 저장 위치: PlayerState 또는 InventoryComponent 메모리 (세션 종료 시 휘발)
- **Phase 2 (웹서버 연동, 추후):**
  - Dedicated Server를 웹서버 Proxy로 두어 HTTP/REST 통신
  - 클라이언트는 절대 웹서버에 직접 접근하지 않음 (보안)
  - 영속 데이터 (레벨, XP, 스킬 해금, 인벤토리)를 로드/세이브 시점에 동기화
  - Runtime 데이터 (HP, Mana, 쿨타임)은 GAS Replication 그대로 유지
- **인벤토리-웹서버 통신 원칙:**
  - `[Client] ←RPC→ [Dedicated Server] ←HTTP/JSON→ [Web API Server] ←→ [DB]`
  - Dedicated Server가 검증 + 웹서버 요청, 클라이언트는 RPC만 호출

### 쿨타임 아키텍처 (표준 패턴)
```
Ability 활성화 → Cooldown GE 적용 → Cooldown Tag 추가
    ↓
WidgetController: RegisterGameplayTagEvent → OnCooldownTagChangedDelegate.Broadcast(AbilityTag, CooldownTag, Percent, Duration)
    ↓
WBP_SkillSlot: Delegate 수신 → Timer 시작 → MI_Cooldown.SetScalarParameterValue("Progress", percent)
```
- WidgetController는 시작/종료 신호만 전달
- 시각적 갱신(Tick)은 WBP 내 Timer가 담당 (Lyra 표준 방식)

## 4. 코딩 규칙 (Coding Standards)
- **Naming:** Unreal Coding Standard 준수 (PascalCase, 접두사 A, U, F 등)
- **GAS:** AttributeSet과 GameplayAbility는 가급적 C++에서 정의하고 데이터는 DataAsset으로 관리.
- **RPC:** 서버-클라이언트 간의 네트워크 부하를 최소화하는 로직 우선.
- **주석:** 주요 로직 및 `UFUNCTION`, `UPROPERTY`에는 한글/영어 주석 필수.
- **인코딩:** 모든 소스 파일 UTF-8 (BOM 포함). `.editorconfig`로 자동 지정.

## 5. AI 작업 가이드
- **작업 시작 전 `CONTEXT.md`를 먼저 읽고 전체 상황을 파악할 것.**
- 코드를 수정하기 전 해당 파일의 전체 구조를 먼저 파악할 것.
- 수정 시 기존 멀티플레이어 동기화 로직이 깨지지 않도록 `Replicated` 변수 및 RPC 체크 필수.
- 새로운 기능을 추가할 땐 먼저 `Source/` 폴더 안의 기존 클래스들과의 의존성을 검토할 것.
- **`CONTEXT.md`에 작업 이력을 갱신할 것.** (사용자 지시: 2026-05-02)

## 6. 작업 이력

### 2026-05-02
- **인코딩 수정:** 프로젝트 14개 소스 파일을 CP949 → UTF-8(BOM)으로 일괄 변환. `.editorconfig` 생성하여 앞으로 UTF-8 기본 지정.
- **쿨타임 로직 검토:** `D1OverlayWidgetController::OnCooldownTagChanged`에서 `Times[0]`만 사용하던 것을 전체 순회하여 최대 TimeRemaining을 찾도록 수정.
- **주석 복구:** `D1OverlayWidgetController.cpp` 내 손상된 한글 주석 4줄 복구.
- **Delegate 시그니처 확장:** `FOnCooldownSignature`를 ThreeParams → FourParams로 변경하여 `Duration` 추가 (GE마다 쿨타임이 다르기 때문).
- **진척도 갱신:** HUD 쿨타임 표시 기능 → Blueprint 작업 대기 중.
- **버그 수정:** `D1SkillMenuWidgetController::EquipSkill`에서 `SelectedAbilityTag`를 `FGameplayTag()`로 초기화하던 코드 제거. 원인: 장착 직후 선택 태그가 날아가서 연속 장착이 불가능했음. 해결: 초기화 코드 제거 (사용자가 다른 아이콘 클릭 시 `AbilitySelected`가 자연스럽게 덮어씀).
- **버그 수정 (슬롯 덮어쓰기):** 스킬을 다른 슬롯으로 변경하거나 동일 슬롯에 덮어씌울 때 기존 어빌리티가 여전히 발동되던 문제 수정. 원인: `ClearSlot` 함수가 완전히 비어 있었고, `ServerEquipAbility`에서 기존 슬롯 태그를 제거하지 않고 그대로 덮어써서 동일 InputTag를 가진 Spec이 2개 이상 생김. 해결: `ClearSlot`에 기존 슬롯 검색 → InputTag 제거 → 상태 Equipped→Unlocked 변경 → 클라이언트 브로드캐스트 로직 구현. `ServerEquipAbility` 시작 부분에 `ClearSlot(SlotTag)` 호출 추가.
