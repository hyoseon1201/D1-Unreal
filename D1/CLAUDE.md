# 프로젝트 개요: D1

탑다운 뷰 멀티플레이 액션 RPG (포트폴리오). GAS 기반 스킬/상태 시스템 + Dedicated Server 멀티플레이어.

- **엔진:** Unreal Engine 5.7 (`C:\Program Files\Epic Games\UE_5.7\`, BuildSettingsVersion.V6, EngineIncludeOrderVersion.Unreal5_7)
- **언어:** C++ (주력), Blueprints (UI/조립)
- **프레임워크:** Gameplay Ability System (GAS), Enhanced Input
- **네트워킹:** Dedicated Server 기반 멀티플레이어
- **백엔드 (Phase 3):** Spring Boot + EC2, 클라이언트는 웹서버에 직접 접근 금지

---

## 1. AI 작업 가이드 (필수 규칙)

### 작업 절차
- 작업 시작 전 이 파일(`context.md`)을 먼저 읽고 전체 상황 파악.
- 코드 수정 전 해당 파일의 전체 구조 먼저 파악. 새 기능은 `Source/` 내 기존 클래스와의 의존성 검토.
- 수정 시 `Replicated` 변수 및 RPC 체크 — 멀티플레이어 동기화가 깨지지 않아야 함.
- **작업 후 이 파일에 이력 갱신.** (사용자 지시: 2026-05-02)

### 빌드/엔진
- **엔진 버전은 반드시 UE_5.7.** 다른 버전 경로 사용 시 빌드 실패 (`BuildSettingsVersion.V6` 오류).
- **UHT 캐시 문제:** `UFUNCTION` 시그니처 변경 후 `.gen.cpp`가 구 시그니처를 참조해 빌드 에러 → VS에서 `Rebuild`, 안 되면 `Intermediate/` 삭제 후 프로젝트 파일 재생성.

### 코드 패턴 (이 프로젝트 고유 규칙)
- **Server RPC는 반드시 `public:` 선언** — WidgetController 등 외부에서 호출. `private:`면 C2248 에러.
- **WidgetController 신규 추가 시 3곳 동시 수정:**
  1. `UD1WidgetController` 상속 클래스 신규 생성 (`.h/.cpp`)
  2. `D1HUD.h/.cpp` — getter, `TObjectPtr<UXxx>` 멤버, `TSubclassOf<UXxx>` Class 변수
  3. `D1AbilitySystemLibrary.h/.cpp` — `UFUNCTION(BlueprintPure)` static getter
- WidgetController 초기화 순서: `SetWidgetControllerParams` → `BindCallbacksToDependencies` → `BroadcastInitialValues`.
- 리프 표시 위젯(목록 아이템 등)은 WidgetController 연결 없이 `SetXxx(Struct)` 직접 주입. 상호작용은 이벤트 디스패처로 부모에 버블업.

### 멀티플레이어/UMG 주의사항
- **Overlap 이벤트는 복제된 다른 플레이어 Pawn에도 발생** → UI 표시 전 `OtherActor == Get Player Pawn(0)` 체크 필수.
- ForEachLoop 도중 순회 중인 배열에 Add 금지 (무한루프). 필터링은 로컬 배열에 담아서.
- 장식용 위젯(Border 이미지 등)은 Visibility = `Hit Test Invisible` — 입력 가로채서 ScrollBox 등이 안 먹는 원인.
- `OnMouseButtonDoubleClick` 등 입력 오버라이드는 루트 Visibility가 `Visible`이어야 동작, Return은 `Handled` 반환.
- **던전 맵 추가 시 `AD1GameStateTown::AllowedDungeonMaps`에 반드시 등록** — 서버가 화이트리스트 기준으로 검증하므로 미등록 맵은 파티 생성/던전 선택이 거부됨.
- **플레이어 식별은 `GetPartyPlayerId()` 사용, `GetPlayerName()` 비교 금지** — 이름은 중복 가능, 표시 전용. Phase 3에서 웹서버 캐릭터 ID로 교체 예정.
- **GAS 데미지/XP 처리에서 `Props.SourceCharacter`는 null 가능** (컨트롤러 없는 가해자: 트랩, 독장판 등) → 역참조 전 `IsValid()` 체크 필수.

## 2. 코딩 규칙
- **Naming:** Unreal Coding Standard (PascalCase, 접두사 A/U/F).
- **GAS:** AttributeSet/GameplayAbility는 C++ 정의, 데이터는 DataAsset.
- **RPC:** 서버-클라이언트 네트워크 부하 최소화 우선.
- **주석:** 주요 로직 및 `UFUNCTION`/`UPROPERTY`에 한글/영어 주석.
- **인코딩:** 모든 소스 파일 UTF-8 (BOM 포함), `.editorconfig` 자동 지정.
- **로깅:** `LogTemp` 사용 금지. `D1.h`에 정의된 프로젝트 카테고리 사용 — `LogD1`(일반), `LogD1Ability`(GAS/스킬), `LogD1Party`(파티), `LogD1Inventory`(인벤토리/장비), `LogD1Travel`(맵 이동/저장·복원), `LogD1UI`(위젯/HUD).
  - 레벨 기준: 정상 흐름 진단용 = `Verbose` (기본 출력 안 됨, 콘솔 `Log LogD1Party Verbose`로 활성화), 주요 이벤트 = `Log`, 실패/거부 = `Warning`, 설정 누락/불가능한 상태 = `Error`.
  - 디버깅 끝난 단계별 trace 로그는 커밋 전에 삭제할 것.

---

## 3. 현재 진행 상황

| # | 작업 항목 | 상태 |
|---|---------|------|
| 1~5 | 캐릭터/능력치/멀티플레이 동기화/메뉴 위젯/HUD 쿨타임 | ✅ 완료 |
| 6 | 인벤토리 시스템 Phase 1 (언리얼 내부) | ✅ 완료 |
| 7 | 웹서버 연동 (로그인/영속화) | ⏳ Phase 3 |
| 8 | 마을/던전 분리 (ClientTravel + GameInstance 임시 저장) | ✅ 완료 |
| 9 | 어빌리티 추가 (ChargeDash, Focus, Heal) | ✅ 완료 |
| 10 | 던전 전투 루프 (몬스터/보스/클리어) | 🔄 진행중 |
| 11 | 마을 파티 시스템 (생성/참가/던전 입장) | 🔄 진행중 — **현재 작업** |

### Phase 3 확정 일정 (2026-06-10 기준, 목표 6/30)

| 기간 | 마일스톤 |
|------|---------|
| ~6/12 | **언리얼 마무리**: 맵 리네임(GoblinCave)+AllowedDungeonMaps 갱신, 파티 풀 루프 2인 테스트 (생성→가입→던전→보스→결과창→마을 복귀), Windows 데디서버 빌드 (Server Target 추가) |
| ~6/16 | DB 스키마 (계정/캐릭터/스탯/인벤토리) + Spring Boot 셋업 + 로그인/JWT + **EC2 첫 배포** (기능 단위 즉시 배포 원칙) |
| ~6/21 | 캐릭터/스탯/인벤토리 저장·로드 API + UE HTTP 연동 (로그인 UI, verify-session) — ⚠️ **최대 리스크 구간**, 밀리면 플랜 B |
| ~6/26 | 크로스 서버 이동: EC2 한 대에 Town/Dungeon 프로세스 2개 **고정 기동**, GameInstance → DB 핸드오프 교체 |
| ~6/30 | 통합 테스트 + 버그픽스 + 데모 영상 촬영 |

**Scope 컷 (20일 일정 성립 조건):**
- 🔪 **ECS/Docker 동적 오케스트레이션 → Phase 4 이월.** MVP는 고정 프로세스 2개(Town:7777, Dungeon:7778), 웹서버는 주소만 내려줌. 설계 문서는 보유 → 면접 어필용
- 🔪 캐릭터 생성 화면 최소화 (커스터마이징 없이 계정당 1캐릭터 자동 생성)
- **플랜 B** (6/21 마일스톤 지연 시): 크로스 서버 포기, 단일 서버 유지 + DB 영속화(재로그인 시 데이터 유지)까지만
- **지켜야 할 척추**: 로그인(JWT) → DB 로드 → Town 접속 → 파티 → Dungeon 크로스 이동(DB 핸드오프) → 클리어 → 복귀 → 재로그인 시 데이터 유지

---

## 4. 진행중: 11번 마을 파티 시스템

마을에서 파티 결성 → 파티장이 던전 입장 트리거.

### C++ 백엔드 (완료)
- **플레이어 식별: `PlayerId` (PlayerName은 표시 전용).** `AD1PlayerState::GetPartyPlayerId()`가 단일 진입점 — 현재 UniqueNetId 문자열(OSS 없으면 PlayerName 폴백), Phase 3에서 웹서버 캐릭터 ID로 교체 시 **이 함수만 수정하면 됨**. 식별/비교에 PlayerName 사용 금지.
- **`FD1PartyMemberInfo`:** `PlayerId` (식별용), `PlayerName` (표시용), `PlayerLevel` (가입 시점 CombatInterface 스냅샷), `bIsReady`
- **`FD1PartyInfo`:** `PartyId`, `LeaderId` (식별용), `LeaderName` (표시용), `PartyName` (방 제목, 빈값이면 서버가 "OOO의 파티" 자동 채움, `MaxPartyNameLength`=30자 초과분 서버에서 잘라냄), `Members`, `MaxMembers` (기본 4, 서버 검증+UI 표시 단일 기준), `SelectedDungeon` (맵 내부 식별자 FString). 헬퍼: `HasMember(PlayerId)`, `IsLeader(PlayerId)`, `IsAllReady()`. `FindPartyByPlayerId()` (구 FindPartyByPlayer 대체)
- **던전 맵 화이트리스트:** `AD1GameStateTown::AllowedDungeonMaps` (기본 `["Dungeon"]`, EditDefaultsOnly) + `IsAllowedDungeonMap()` (BlueprintPure). `ServerCreateParty`/`ServerSelectDungeon` 진입점 2곳에서 검증 — 조작된 클라이언트의 임의 맵 입장 차단. `StartDungeonForParty`는 검증된 `SelectedDungeon`만 읽으므로 Travel 경로 전체 안전.
- **`AD1GameStateTown`:** `Parties` (TArray, `ReplicatedUsing=OnRep_Parties`) — 변경 시 맵 내 전 클라이언트 자동 복제 (UI 존재 여부 무관). `OnRep_Parties()` → `OnPartiesChangedDelegate` 브로드캐스트.
  - 서버 전용: `ServerCreateParty(LeaderName, LeaderLevel, DungeonMap, PartyName)`, `ServerDisbandParty`, `ServerJoinParty(PartyId, PlayerName, PlayerLevel)` (MaxMembers 검증, 중복 가입 방지), `ServerLeaveParty` (파티장 탈퇴 시 해산), `ServerSetReady`, `ServerSelectDungeon`, `FindPartyByPlayer` (일반/const), `GeneratePartyId`
- **`AD1GameModeTown`:** `AreAbilitiesAllowed() → false` (마을 전투 차단), `StartDungeonForParty(LeaderPC)` — 파티원 전원 `TravelToMap`
- **`AD1PlayerController` Server RPC (public, BlueprintCallable):** `Server_CreateParty(DungeonMap, PartyName)`, `Server_JoinParty(PartyId)` (둘 다 CombatInterface로 레벨 읽어 전달), `Server_LeaveParty`, `Server_SetReady`, `Server_SetSelectedDungeon`, `Server_StartDungeon`. `ShowDungeonEntryUI()` — 포탈 Overlap 시 호출, 인스턴스 캐싱으로 중복 생성 방지
- **`UD1DungeonPartyWidgetController`:**
  - `OnPartiesUpdated` 델리게이트 (`OnRep_Parties` → Broadcast 체인), `BroadcastInitialValues()` (위젯 열릴 때 현재 스냅샷 1회 — UI 닫혀있는 동안 놓친 변경 동기화)
  - 액션: `CreateParty`, `JoinParty`, `LeaveParty`, `SetReady`, `SelectDungeon`, `StartDungeon`
  - Pure 쿼리: `IsInParty()`, `IsPartyLeader()`, `GetMyParty()` (미소속 시 PartyId=INDEX_NONE)

### UI 아키텍처 (구현 중)
```
WBP_DungeonEntry (Full Screen, CreateWidget+AddToViewport, 닫기는 RemoveFromParent)
 ├─ WBP_DungeonList (좌측, 하드코딩 던전 목록)
 ├─ Widget Switcher (우측)
 │   ├─ Index 0: WBP_EmptyState (던전 미선택)
 │   ├─ Index 1: WBP_PartyInfo (파티 소속 시) — 정적 4슬롯
 │   └─ Index 2: WBP_PartyList (던전 선택 + 미소속)
 │       └─ Scroll Box → WBP_PartyListItem (동적 생성)
 └─ WBP_CreatePartyPopup (기본 Collapsed, Visibility 토글 모달)
```
- **`RefreshPartyPanel()` 중앙 분기 (Entry):** `IsInParty()?` → Index1 (`GetMyParty()` → `UpdatePartyInfo()`) | `SelectedDungeonMapName` 비어있지 않으면 → Index2 (필터링) | else → Index0
  - 호출처: `OnPartiesUpdated` (CachedParties를 SET으로 통째 교체 후), `OnDungeonSelected` (SelectedDungeonMapName 갱신 후)
  - `CachedParties`는 PartyList 필터링 전용 / PartyInfo는 `GetMyParty()` 전용 — 역할 분리
  - 필터링: ForEachLoop(CachedParties) → 로컬 `FilteredParties`에 Add → Completed에서 ScrollBox 갱신
- **`WBP_PartyInfo`:** Slot_0~3 정적 배치 (MaxMembers=4 고정). `UpdatePartyInfo(FD1PartyInfo)`: 전 슬롯 Hidden → ForEachLoop(Members) + Switch on Int(ArrayIndex)로 슬롯 직접 접근 → `SetMemberData(MemberInfo, bIsLeader)` + Visible. `bIsLeader`는 `PlayerName == LeaderName` 비교로 파생 (구조체 필드 불필요)
- **`WBP_PartyListItem`:** `SetPartyInfo(FD1PartyInfo)` 주입 (CachedPartyInfo 변수에 저장). 더블클릭(`OnMouseButtonDoubleClick` 오버라이드) → 인원 체크 → `OnJoinRequested(PartyId)` 버블업 2단 (PartyList → Entry) → Entry가 `JoinParty(PartyId)`. Index 1 전환은 복제 후 RefreshPartyPanel이 자동 처리
- **`WBP_CreatePartyPopup`:** `OnPartyCreateConfirmed(PartyName)` 디스패처, Entry가 Construct에서 Bind → `CreateParty(SelectedDungeonMapName, PartyName)` 호출

### 관련 파일
`D1GameStateTown.h/.cpp`, `D1GameModeTown.h/.cpp`, `D1PlayerController.h/.cpp`, `D1DungeonPartyWidgetController.h/.cpp` (신규), `D1HUD.h/.cpp`, `D1AbilitySystemLibrary.h/.cpp`

---

## 5. 진행중: 10번 던전 전투 루프

- **완료:** 보스 사망 감지 (`D1Enemy::Die()` HasAuthority + `bIsBoss` → `GameModeDungeon::OnBossDefeated()`) → 전 PC 순회 `ClientShowDungeonResult(TArray<FText>)` Client RPC → `D1DungeonResultWidgetController` → `WBP_DungeonResult` 표시, "Return to Town" 버튼. 아이템 목록을 RPC에 실어 보내 위젯 생성 타이밍 이슈 해결.
- **미완:** 몬스터 AI, 보상 시스템.
- 관련 파일: `D1GameModeDungeon.h/.cpp`, `D1DungeonResultWidgetController.h/.cpp`, `D1Enemy.h/.cpp`, `D1PlayerController.h/.cpp`

---

## 6. 완료된 시스템 요약

### 인벤토리 (6번, Phase 1 완료)
- `UD1ItemData` (PrimaryDataAsset 메타데이터), `FD1InventoryItem` (슬롯), `UD1InventoryComponent` (PlayerState 부착, Server RPC: Use/Move/Discard)
- 소비 아이템 GE 연동, 장비 장착/탈착 (`EquipEffect` GE Apply/Remove, `WBP_EquipmentSlot` 7개), 탈착 시 인벤토리 자동 반환
- 멀티플레이어 2인 테스트 완료. 데이터는 메모리 (세션 휘발 — Phase 3에서 웹서버 영속화)

### 마을/던전 분리 (8번 완료) — ClientTravel 데이터 유지
- **문제:** `ClientTravel` 시 PlayerState 새로 생성 → 스탯 리셋
- **해결: `UD1GameInstance` 임시 저장 패턴**
  - `AD1PlayerController::PreClientTravel` 오버라이드 → Travel 직전 자동 저장 (AttributePoints, Level, XP, Primary 4종 — `bAbilitySystemInitialized`는 저장 금지)
  - `AD1Hero::PossessedBy` 순서: `InitializeDefaultAttributes()` → `RestoreTravelDataIfNeeded()` (Override) → `AddCharacterAbilities()` (항상 재등록, 중복 Spec은 자동 스킵)
  - OnRep은 값이 실제로 달라졌을 때만 호출 (레벨업 이펙트 중복 방지)
- **한계:** 같은 프로세스 내에서만 유효. **별도 데디서버 간 이동에는 사용 불가** → 웹서버/DB 핸드오프로 교체 예정 (Phase 3)
- 관련 파일: `D1GameInstance.h/.cpp`, `D1PlayerController.h/.cpp`, `D1PlayerState.h/.cpp`, `D1Hero.cpp`, `DefaultEngine.ini` (GameInstanceClass)

### 어빌리티 (9번 완료)
- **보유:** 근접 기본 공격 (Instant GE + LineTrace), 광역 스킬 (Projectile → 폭발 Area GE), ChargeDash, Focus, Heal
- **ChargeDash:** `UDashToLocation` AbilityTask (서버에서 CMC Velocity 설정, MOVE_Flying, 70% 미만 이동 시 OnHitWall). `GA_ChargeDash`: Dash+Montage 동시 실행, OnDashComplete → Montage Section Jump (Slam). `ActivationOwnedTags`로 AutoRun 입력 블록.
- **Focus:** Duration 10초 GE, AttackPower +30, 쿨다운 12초, GameplayCue 오라
- **Heal:** Instant GE Health +200, Mana Cost 30, 쿨다운 10초
- **패시브 4종 예정:** 불굴의 의지 / 마나 재생 / 강인함 / 전투 본능 (Infinite GE + OnSpawn Activation)

---

## 7. 서버 아키텍처 설계 (Phase 3)

### 통신 원칙
- `[Client] ←RPC→ [Dedicated Server] ←HTTP/JSON→ [Spring Boot] ←→ [DB]`
- 클라이언트는 웹서버에 게임 패킷 절대 직접 전송 안 함 (로그인/매칭만 HTTPS)
- 영속 데이터(레벨, XP, 인벤토리)는 로드/세이브 시점 동기화, Runtime 데이터(HP/Mana/쿨타임)는 GAS Replication 유지

### 접속 플로우
1. `POST /login` → JWT 발급 → `GET /characters` → `POST /matchmaking/town`
2. 웹서버가 IP/Port/SessionToken(1회용) 응답 → 클라이언트 `ClientTravel` 직접 접속
3. 데디서버 → 웹서버 `POST /verify-session` (별도 API Key 인증) → DB 로드 → GAS 초기화

### 확장 전략
- 마을: 채널(샤드) 분리, 1채널 50~100명. 같은 바이너리를 `-TownServer`/`-DungeonServer` 인자로 모드 분리 (마을: AI/전투 비활성, 던전: 풀 전투 60Hz)
- 던전: Stateless 인스턴스 — 입장 시 생성, 클리어 시 파괴
- **오케스트레이션 (포트폴리오 스코프): Docker + AWS ECS Fargate.** Spring Boot가 AWS SDK로 RunTask/StopTask 호출, Redis로 세션 매핑. Town은 고정 1~2개, Dungeon만 동적. (Agones/GameLift는 그 다음 단계)
- 웹서버는 기능 단위로 EC2 즉시 배포하며 UE5와 통합 테스트 (마지막에 몰아서 배포 금지)

### Scope 확정 (포트폴리오)
- 매치메이킹 없음, 이펙트/사운드는 무료 에셋, 웹서버는 로그인/캐릭터/인벤토리 저장·불러오기만
- Linux 서버 빌드는 Phase 3 (Windows 빌드 먼저)

---

## 8. 작업 이력

상세 내역은 `History/` 폴더의 날짜별 문서 참조.

| 날짜 | 요약 |
|------|------|
| 05-02~03 | 초기 셋업, GameDataReference.md, 아이템 데이터 구조 기획 |
| 05-04~08 | 인벤토리 WidgetController/UI/사용/장비 장착·탈착 |
| 05-11 | 전체 로드맵 + 서버 아키텍처 설계 확정 |
| 05-12 | 인벤토리 Phase 1 완료, 멀티 2인 테스트. 어빌리티 기획 (`Docs/AbilityDesign_2026-05-12.md`) |
| 05-20 | ChargeDash 완성 (DashToLocation AbilityTask) |
| 05-25 | Focus/Heal 완성, Secondary Init GE Instant 변경, RecalculateSecondaryAttributes |
| 05-26 | ClientTravel PlayerState 리셋 해결 (GameInstance 패턴), 던전 결과창, 보스 사망 감지 |
| (날짜 미상) | 파티 시스템 C++ 1차 (GameStateTown, GameModeTown) |
| 06-09~10 | 파티 구조체 확장 (PartyName/MaxMembers/PlayerLevel), DungeonPartyWidgetController, UI 아키텍처 설계, 포탈 Overlap 버그 수정 |
| 06-10 | 파티 UI BP 구현: PartyInfo 4슬롯 (Switch on Int), PartyList 필터링 (로컬 배열 — 순회 중 Add 무한루프 수정), GetMyParty/CachedParties 역할 분리, 더블클릭 가입 설계 |
| 06-10 (코드 품질) | 코드베이스 리뷰 후 우선순위 3건 개선. ① null 크래시 수정: `D1AttributeSet.cpp`의 `ShowFloatingText`/`PostGameplayEffectExecute`(IncomingXP)/`SendXPEvent`에서 `Props.SourceCharacter` IsValid 가드 (컨트롤러 없는 가해자 시 크래시). `D1Hero.cpp`의 `GetAttributePointsReward`/`GetSkillPointsReward`에 `LevelupInformation.IsValidIndex` 가드 (범위 초과 시 보상 0), `FindLevelForXP`에 LevelUpInfo null 가드. ② 로그 정리: 전용 카테고리 6종 신설 (`D1.h`), UE_LOG 189→90개, LogTemp 0건. 입력/타격마다 찍히던 스팸 로그 삭제, 정상 흐름은 Verbose, 실패만 Warning/Error. ③ 서버 검증: `AllowedDungeonMaps` 화이트리스트 + `IsAllowedDungeonMap()`, `ServerCreateParty`/`ServerSelectDungeon` 검증, PartyName 30자 제한. 리뷰에서 발견된 미해결 항목: 인벤토리 스택 미구현 (MaxStack 무시), `DynamicAbilityTags` deprecation 6건 (차기 엔진에서 컴파일 에러 예정, `GetDynamicSpecSourceTags()`로 교체 필요), `Die()` 베이스 HasAuthority 가드 부재. |
| 06-10 (전략) | Phase 3 일정 확정 (~6/30, ECS 컷). 포트폴리오 전략 문서 작성: `Docs/PortfolioStrategy_2026-06-10.md` — 어필 포인트 5종(상태 동기화 진화, 서버 권위, 식별자 추상화, GAS, 풀스택), 학습 우선순위(①부하테스트+수치화 ②FastArraySerializer ③ECS), README "문제→접근→결과" 구조 권장. 면접 대비 문서: `Docs/InterviewPrep_StateSync_2026-06-10.md` — 상태 동기화 스토리 타임라인 5단계 + 예상 질문 8개/모범 답변 + 심화 키워드 |
| 06-10 (PlayerId 전환) | 파티 식별자를 PlayerName → PlayerId로 전환. `AD1PlayerState::GetPartyPlayerId()` 추상화 신설 (현재 UniqueNetId 문자열, OSS 없으면 PlayerName 폴백, Phase 3에서 웹서버 캐릭터 ID로 이 함수만 교체). `FD1PartyMemberInfo.PlayerId` / `FD1PartyInfo.LeaderId` 필드 추가 (Name 필드는 표시 전용 유지). `HasMember`/`IsLeader`/`FindPartyByPlayerId`/Server 함수 시그니처(`ServerCreateParty`에 LeaderId, `ServerJoinParty`에 PlayerId 등) ID 기준 전환. `StartDungeonForParty` 멤버 룩업, WidgetController `GetLocalPlayerId()` 전환. UI BP는 PlayerName 표시 그대로라 수정 불필요. |
