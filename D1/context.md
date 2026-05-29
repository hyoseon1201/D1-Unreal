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
| 6 | 인벤토리 시스템 (언리얼 납부) | ✅ 완료 |
| 7 | 웹서버 연동 (로비/인벤토리/스탯 영속화) | ⏳ Phase 3 이월 |
| 8 | 로비/던전 분리 (Gameplay Mode) | ✅ 완료 (GameInstance 임시 저장으로 데이터 유지) |
| 9 | 다양한 어빌리티 추가 개발 | ✅ 완료 (ChargeDash, Focus, Heal) |

### 6번 상세 (완료)
- **Phase 1 (언리얼 납부, 완료):** 웹서버 없이 언리얼 낶에서만 동작하는 인벤토리 시스템 구축
  - `UD1ItemData` (UPrimaryDataAsset): 아이템 메타데이터 (이름, 아이콘, 설명, 최대중첩, 효과 등)
  - `FD1InventoryItem` (USTRUCT): 인벤토리 슬롯 (ItemID, Count, InstanceData)
  - `UD1InventoryComponent` (UActorComponent): PlayerState에 부착, 실제 인벤토리 로직 담당
    - `ServerUseItem()`, `ServerMoveItem()`, `ServerDiscardItem()` (RPC)
    - `AddItem()`, `RemoveItem()` (서버 전용)
  - `WBP_Inventory` / `WBP_InventorySlot` (Blueprint): UI (클이언트)
  - 데이터 저장 위치: PlayerState 또는 InventoryComponent 메모리 (세션 종료 시 휘발)
- **Phase 2 (웹서버 연동, 예정):**
  - Dedicated Server를 웹서버 Proxy로 두어 HTTP/REST 통신
  - 클라이언트는 절대 웹서버에 직접 접근하지 않음 (보안)
  - 영속 데이터 (레벨, XP, 스킬 해금, 인벤토리)를 로드/세이브 시점에 동기화
  - Runtime 데이터 (HP, Mana, 쿨타임)은 GAS Replication 그대로 유지
- **인벤토리-웹서버 통신 원칙:**
  - `[Client] ←RPC→ [Dedicated Server] ←HTTP/JSON→ [Web API Server] ←→ [DB]`
  - Dedicated Server가 검증 + 웹서버 요청, 클라이언트는 RPC만 호출
- **Phase 1 완료:** 웹서버 없이 언리얼 낶에서만 동작하는 인벤토리 시스템 전체 완성
  - 소비 아이템 사용 (GE 연동, HP 회복 확인)
  - 장비 장착/탈착 (Server RPC, EquipEffect GE Apply/Remove)
  - 장착 슬롯 UI (`WBP_EquipmentSlot` 7개: Weapon~Ring)
  - 장착 장비 아이콘 표시 (`OnEquippedItemsUpdated` → 실시간 갱신)
  - 탈착 시 인벤토리 자동 반환 (`UnequipItem` → `AddItem` → 첫 빈 슬롯)
  - 멀티플레이어 테스트 완료 (Net Mode: Play as Client, 2인 동시 접속 정상)

### 9번 상세 (어빌리티 추가 개발)
- **1. ChargeDash (대시/회피) — 구현 완료:**
  - **C++:** `UDashToLocation` (AbilityTask) 신규 생성 (`AbilitySystem/AbilityTask/DashToLocation.h/.cpp`)
    - 서버에서만 `CMC->Velocity`를 설정 (Direction × Speed). 클라이언트는 CMC Replicate/보간으로 자연스럽게 따라감.
    - `MOVE_Flying` 모드로 지형/경사 영향 차단, `GroundFriction=0`, `BrakingDecelerationWalking=0`으로 감속 제거.
    - Duration 후 실제 이동 거리가 설정 거리의 70% 미만이면 `OnHitWall` 발송, 아니면 `OnDashComplete` 발송.
    - AbilityTask 생명주기에 종속된 Timer 사용 → Ability 중단(스턴/죽음) 시 자동 정리.
    - 클라이언트에서도 `OnDashComplete`/`OnHitWall` delegate 발송 → Montage Jump 등 로컬 처리 가능.
  - **BP (`GA_ChargeDash`):**
    - `ActivateAbility` 즉시 `SetActorRotation` (Teleport=true)로 마우스 방향 즉시 회전.
    - `CreateDashToLocation` (Async) + `PlayMontageAndWait` (Async)를 **동시에** 실행.
    - `OnDashComplete` → `Get Anim Instance` → `Montage Jump to Section Name` (Slam) 연결.
    - `EndAbility`는 오직 `PlayMontageAndWait`의 `OnCompleted/BlendOut/Interrupted`에서만 호출.
  - **Animation (`AM_ChargeDash`):**
    - `EnableRootMotion = false` (CMC Velocity 기반 이동이므로 RootMotion 불필요).
    - Section 구조: Charge (돌진 자세) → Slam (내리치기, `Next=None`) → Recover (사용 안 함).
    - `AN_SlamImpact` AnimNotify에서 `CauseDamage` (HasAuthority guard).
  - **입력 블록:**
    - `GA_ChargeDash`의 `ActivationOwnedTags`에 `Player.Block.InputPressed`, `Player.Block.InputHeld` 추가.
    - `D1PlayerController.cpp` 수정: `AutoRun()` 및 `AbilityInputTagReleased()`에서 태그 체크 → Ability 실행 중 우클릭 AutoRun 시작/진행 차단.
  - **결론:** 리븐 E 스타일의 "자세 잡고 돌진 → 내리치기 → 바로 원위치" 구현 완료. 거리/시간은 Blueprint에서 조절 가능 (`Speed = Distance / Duration`).

  - **2. Focus (버프) — 구현 완료:**
    - **효과:** Duration 10초, AttackPower Add +30 (Secondary Attribute 직접 버프)
    - **GE:** `GE_Focus` (Duration GE, `Status.Buff.Focus` Granted Tag)
    - **BP (`GA_Focus`):** `ApplyGameplayEffectToOwner` + `PlayMontageAndWait`, 입력 블록 태그 적용
    - **GameplayCue:** `GameplayCue.Buff.Focus` 태그 등록 (지속 오라용)
    - **Cooldown:** `GE_Cooldown_Focus` (12초)

  - **3. Heal (자기 회복) — 구현 완료:**
    - **효과:** Instant GE, Health Add +200, Mana Cost -30
    - **GE:** `GE_Heal` (Instant), `GE_Cost_Mana_Heal`, `GE_Cooldown_Heal` (10초)
    - **BP (`GA_Heal`):** `ApplyGameplayEffectToOwner` (HasAuthority) + `PlayMontageAndWait`
    - **애니메이션:** 짧은 캐스팅 몽타주 (기존 애니메이션 재활용)

  - **4. 패시브 4개 — 예정 (로비/던전 이후 또는 병행):**
    - **불굴의 의지:** HP 30% 이하 IncomingDamage -20%
    - **마나 재생:** 초당 Mana +2 (Periodic Infinite GE)
    - **강인함:** MaxHealth +100 (Infinite GE)
    - **전투 본능:** AttackPower +10% (Infinite GE)
    - **구현 방식:** Startup Abilities 등록 + Infinite GE 자동 적용 (`OnSpawn` Activation Policy)

- **이미 구현된 어빌리티:**
  - 근접 기본 공격 (Instant GE + LineTrace)
  - 광역 스킬 (Projectile → 폭발 Area GE)
  - ChargeDash (Dash + CMC Velocity 기반 이동)
  - Focus (버프)
  - Heal (자기 회복)
- **어빌리티 완료 후 산출물:**
  - 기동/생존/버프 시스템을 포함한 전투 루프 완성
  - Skill Menu에 5~6개 스킬 아이콘 + 쿨타임 표시
  - GAS `Cooldown`, `Cost`, `GameplayCue`, `Ability Task` 학습 완료

### 8번 상세 (로비/던전 분리 — 구현 완료)
- **문제:** PIE `ClientTravel` 시 새 `PlayerState`가 생성되어 `AttributePoints`, `Primary Attributes`, `bAbilitySystemInitialized`가 기본값으로 리셋됨
- **원인:** PIE에서는 `ClientTravel`이 기존 `PlayerState`를 유지하지 않고 새로 생성함 (Standalone/Listen Server에서도 동일)
- **해결책:** `GameInstance` 임시 저장 패턴 (PIE에서도 `GameInstance`는 살아있음)
  - **`UD1GameInstance` 신규 생성:** `SavePlayerStateData` / `RestorePlayerStateData` / `ClearSavedData`
    - 저장 항목: `bAbilitySystemInitialized`, `AttributePoints`, `Level`, `XP`, `Strength`, `Intelligence`, `Dexterity`, `Luck`
  - **`AD1PlayerController::PreClientTravel` 오버라이드:** Engine이 `ClientTravel` 직전에 자동 호출 → `PlayerState` + `AttributeSet` 데이터를 `GameInstance`에 저장
  - **`AD1Hero::PossessedBy` 수정:** `InitAbilityActorInfo()` 직후 `D1PS->RestoreTravelDataIfNeeded()` 호출
    - `InitializeDefaultAttributes()` **실행 전**에 `bAbilitySystemInitialized`와 Primary Attribute를 복원
    - `bInit=TRUE` 복원 시 → `InitializeDefaultAttributes()` **스킵** (Primary GE 덮어쓰기 방지)
    - 대신 `Secondary GE + Vital GE`만 **재적용** (새 `AttributeSet`에는 이들이 필요함)
  - **`DefaultEngine.ini`:** `GameInstanceClass=/Script/D1.D1GameInstance` 추가 (Project Settings에서 설정)
- **결과:** Town → Dungeon 이동 시 스탯 투자한 Primary Attribute(Strength 등), 남은 AttributePoints, `bAbilitySystemInitialized` 상태가 정상 유지됨
- **한계:** `GameInstance` 임시 저장은 세션 내 맵 이동용. 세션 종료 후에는 웹서버 영속화(Phase 3) 필요.
- **관련 파일:**
  - `D1GameInstance.h/.cpp` (신규)
  - `D1PlayerController.h/.cpp` (`PreClientTravel` 추가)
  - `D1PlayerState.h/.cpp` (`RestoreTravelDataIfNeeded()` 추가)
  - `D1Hero.cpp` (`PossessedBy` 복원 호출, Secondary+Vital 재적용)
  - `D1GameModeBase.cpp` (`PostLogin` 디버그 로그)
  - `DefaultEngine.ini` (`GameInstanceClass` 설정)

### 6주 완성 로드맵 (현실적인 Scope)
- **개발자 경험:** Spring Boot 1년 (오랜만에 복귀), EC2 직접 배포 경험 있음
- **일정 (포트폴리오용 Scope 재조정):**
  - **Week 1 (완료):** 어빌리티 시스템 완성 (ChargeDash, Focus, Heal) + PIE 테스트
  - **Week 2 (현재):** 마을/던전 분리 (GameMode, ClientTravel, 입장/퇴장 흐름)
  - **Week 3:** 던전 내 전투 루프 (몬스터 스폰, AI, 보상 시스템)
  - **Week 4:** 통합 테스트 + Windows Dedicated Server 빌드 + 데모 영상 촬영
  - **Phase 3 (이월):** Spring Boot 웹서버 (로그인, 영속화), Linux 서버 빌드
- **Scope 축소 확정:**
  - 매치메이킹 없음 (혼자 입장)
  - Linux 서버 빌드 Phase 3로 이월
  - 이펙트/사운드는 언리얼 마켓플레이스 무료 에셋 사용
  - Docker 없이 EC2에 jar 직접 배포
  - 웹서버 기능: 로그인/캐릭터 정보/인벤토리 저장·불러오기만 (결제, 길드, 채팅 등 제외)

### 7~8번 상세 (서버 아키텍처 설계 확정)
- **클-서 통신 구조:**
  - 클라이언트는 절대 웹서버에 직접 게임 패킷을 별내지 않음
  - 로그인/매칭만 웹서버(HTTPS)를 통해 처리
  - 실제 게임 플레이는 클라이언트 ↔ Dedicated Server가 직접 UE5 Replication/RPC로 통신
- **유저 플로우:**
  1. 클라이언트 → 웹서버: `POST /login` → JWT 발급
  2. 클라이언트 → 웹서버: `GET /characters` (캐릭터 목록)
  3. 클라이언트 → 웹서버: `POST /matchmaking/town` (캐릭터 선택)
  4. 웹서버 → 클라이언트: IP, Port, SessionToken (일회용) 응답
  5. 클라이언트 → Dedicated Server: `ClientTravel`로 직접 접속 (Token 전달)
  6. Dedicated Server → 웹서버: `POST /verify-session` (Token 검증)
  7. 검증 완료 → DB에서 영속 데이터 로드 → GAS AttributeSet 초기화 → 게임 시작
- **마을 서버 vs 던전 서버 확장 전략:**
  - 마을도 여러 채널(샤드)로 분리: 1채널당 50~100명 (Lost Ark/MapleStory 방식)
  - 같은 UE5 서버 바이너리라도 커맨드라인 인자(`-TownServer`/`-DungeonServer`)로 모드 분리
    - 마을 모드: AI 스폰 최소화, Physics Tick 낮춤, 전투 로직 비활성화
    - 던전 모드: 풀 전투, AI/몬스터 스폰 활성화, 60Hz Tick
  - K8s 등 컨테이너 환경에서 리소스 할량을 다르게 설정 (마을: 4Gi/2CPU, 던전: 1.5Gi/0.5CPU)
  - 던전은 Stateless 인스턴스: 입장 시 생성, 클리어/퇴장 시 파괴 (K8s HPA 연동)
- **주의사항:**
  - 웹서버가 게임 패킷을 프록시하지 않음 (HTTP는 실시간 게임 통신에 부적합)
  - SessionToken은 1회용, 데디 서버 인증 후 즉시 폐기
  - 데디 서버 → 웹서버 통신 시 낮 API Key로 인증 (외부 노출 금지)

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
  - `History/2026-05-06_WorkLog.md` — GameState 기반 `GetItemData` 구조 개선, 인벤토리 상호작용 구현 (좌클릭 상세정보, 모달 팝업, 아이템 아이콘 표시 확인)
  - `History/2026-05-07_WorkLog.md` — 아이템 사용 기능 완성 (`UseItemInternal` GE 연동, WBP_ItemDetail UI 연동, 초기화 시점 이슈 해결)
  - `History/2026-05-08_WorkLog.md` — 장비 장착/탈착 시스템 구현 (`ServerEquipItem`/`ServerUnequipItem`, `FD1EquippedItem`, `EquipEffect` GE Apply/Remove, WidgetController `EquipItem`/`UnequipItem` 추가)
  - `History/2026-05-11_WorkLog.md` — 전체 프로젝트 로드맵 수립 (12주), 서버 아키텍처 설계 확정 (로그인→매칭→데디서버 접속 흐름, 마을/던전 서버 확장 전략, K8s 리소스 분리)
  - `History/2026-05-12_WorkLog.md` — 장비 장착/탈착 UI 및 기능 완성, 멀티플레이어 2인 테스트 완료, 인벤토리 Phase 1 마무리
  - `Docs/AbilityDesign_2026-05-12.md` — 추가 어빌리티 3개 기획서 (Dash/Heal/StunStrike), 6주 완성 로드맵 확정
  - `History/2026-05-20_WorkLog.md` — `UDashToLocation` AbilityTask 구현 완료, `GA_ChargeDash` 리븐 E 스타일 대시 완성, `D1PlayerController` AutoRun 태그 블록 수정, 몽타주 Section Jump 적용
  - `History/2026-05-25_WorkLog.md` — `GA_Focus` 버프 Ability 완성 (AttackPower +30, 10초 지속, GameplayCue 태그 등록), `GA_Heal` 자기 회복 Ability 완성 (Instant GE, Health +200), Secondary Init GE를 Instant Duration으로 변경하여 장비/버프 Add Modifier 정상 작동, `RecalculateSecondaryAttributes` 함수 추가 (스탯 투자 시 Secondary 재계산)
  - `History/2026-05-26_WorkLog.md` — `ClientTravel` PlayerState 리셋 버그 해결. `D1GameInstance` 임시 저장 패턴 적용 (`SavePlayerStateData`/`RestorePlayerDataIfNeeded`). `PreClientTravel` 오버라이드로 Travel 직전 자동 저장. `PossessedBy`에서 복원 → `bAbilitySystemInitialized` 및 Primary Attributes 유지. Secondary+Vital GE는 새 AttributeSet에 재적용. Town ↔ Dungeon 데이터 유지 확인.
