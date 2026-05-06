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

### 6번 상세 (현재 집중 작업)
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

## 4. 코딩 규칙 (Coding Standards)
- **Naming:** Unreal Coding Standard 준수 (PascalCase, 접두사 A, U, F 등)
- **GAS:** AttributeSet과 GameplayAbility는 가급적 C++에서 정의하고 데이터는 DataAsset으로 관리.
- **RPC:** 서버-클리언트 간의 네트워크 부하를 최소화하는 로직 우선.
- **주석:** 주요 로직 및 `UFUNCTION`, `UPROPERTY`에는 한글/영어 주석 필수.
- **인코딩:** 모든 소스 파일 UTF-8 (BOM 포함). `.editorconfig`로 자동 지정.

## 5. AI 작업 가이드
- **작업 시작 전 `CONTEXT.md`를 먼저 읽고 전체 상황을 파악할 것.**
- 코드를 수정하기 전 해당 파일의 전체 구조를 먼저 파악할 것.
- 수정 시 기존 멀티플레이어 동기화 로직이 깨지지 않도록 `Replicated` 변수 및 RPC 체크 필수.
- 새로운 기능을 추가할 땐 먼저 `Source/` 폴다 안의 기존 클래스들과의 의존성을 검토할 것.
- **`CONTEXT.md`에 작업 이력을 갱신할 것.** (사용자 지시: 2026-05-02)

## 6. 작업 이력
- 상세 작업 내역은 `History/` 폴다의 날짜별 문서를 참조
  - `History/2026-05-02_WorkLog.md`
  - `History/2026-05-03_WorkLog.md` — GameDataReference.md 생성 (Attribute/Skill/Class/Inventory 데이터 스키마 정리)
  - `History/2026-05-03_WorkLog_Item.md` — 아이템 데이터 구조 및 기획서 작성 (`EItemType`, `EEquipmentSlot`, `UD1ItemData`, `UD1ItemRegistry`, `GetItemData()` 구현)
  - `History/2026-05-04_WorkLog.md` — `D1InventoryWidgetController` 구현 (`BroadcastInitialValues`, `BindCallbacks`, `UseItem`, `MoveItem`, `DiscardItem`, `GetItemData`, `OnInventoryUpdated` 델리게이트)
  - `History/2026-05-05_WorkLog.md` — 인벤토리 UI 구현 (`WBP_Inventory`, `WBP_InventorySlot`, UniformGridPanel 동적 생성 60슬롯, 위젯 컨트롤러 연동, 닫기/열기 토글)
