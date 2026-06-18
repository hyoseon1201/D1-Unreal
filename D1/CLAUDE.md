# 프로젝트 개요: D1

탑다운 뷰 멀티플레이 액션 RPG (포트폴리오). GAS 기반 스킬/상태 시스템 + Dedicated Server 멀티플레이어.

- **엔진:** Unreal Engine 5.7.4 소스 빌드 (`D:\UnrealEngineSource\UnrealEngine`, BuildSettingsVersion.V6, EngineIncludeOrderVersion.Unreal5_7)
- **언어:** C++ (주력), Blueprints (UI/조립)
- **프레임워크:** Gameplay Ability System (GAS), Enhanced Input
- **네트워킹:** Dedicated Server 기반 멀티플레이어
- **백엔드 (Phase 3):** Spring Boot + MySQL, GCP (Cloud SQL + GCE/Cloud Run), Redis 없이 MVP 시작

---

## 1. AI 작업 가이드 (필수 규칙)

### 작업 절차
- 작업 시작 전 이 파일(`CLAUDE.md`)을 먼저 읽고 전체 상황 파악.
- 코드 수정 전 해당 파일의 전체 구조 먼저 파악. 새 기능은 `Source/` 내 기존 클래스와의 의존성 검토.
- 수정 시 `Replicated` 변수 및 RPC 체크 — 멀티플레이어 동기화가 깨지지 않아야 함.
- **작업 후 이 파일에 이력 갱신.** (사용자 지시: 2026-05-02)

### 빌드/엔진
- **엔진 버전은 반드시 UE 5.7.4 소스 빌드.** 경로: `D:\UnrealEngineSource\UnrealEngine`. Launcher 버전은 Server Target 미지원.
- **UHT 캐시 문제:** `UFUNCTION` 시그니처 변경 후 `.gen.cpp`가 구 시그니처를 참조해 빌드 에러 → VS에서 `Rebuild`, 안 되면 `Intermediate/` 삭제 후 프로젝트 파일 재생성.
- **패키징 배치파일** (`D:\D1\`): `BuildServer.bat`, `BuildClient.bat`, `BuildAll.bat` — UAT 사용, `-map=/Game/Maps/Town+/Game/Maps/GoblinCave` 명시 필수 (생략 시 맵 누락).
- **실행 배치파일** (`D:\D1\`): `RunServer.bat` (port 7777), `RunClients.bat` (클라 2개), `RunClient1.bat`.
- **빌드 출력:** 서버 → `D:\D1\Build\Server\WindowsServer\D1Server.exe`, 클라 → `D:\D1\Build\Client\Windows\D1.exe`.
- **새 던전 맵 추가 시 필수 3곳:** ① `BuildServer.bat`/`BuildAll.bat`의 `-map=` 파라미터에 추가, ② `AD1GameStateTown::AllowedDungeonMaps`에 등록, ③ `DefaultGame.ini`의 `MapsToCook`에 추가 (UAT가 직접 읽진 않지만 에디터 쿡에서 참조).

### GAS 아키텍처 (현재 확정)
- **Primary Attributes (Strength/Intelligence/Dexterity/Luck):** Infinite GE + Additive. 직업별 초기값을 ScalableFloat로 보관, 스폰 시 Add로 적용. `AS->SetXxx()` 직접 호출 금지 — GE로만 변경.
- **Secondary Attributes (MaxHealth/MaxMana/AttackPower 등):** Infinite GE + Additive. MMC가 Primary를 읽어 계산. 레벨업/스폰 시 `RecalculateSecondaryAttributes()` 수동 재적용 필요 (Primary가 GAS dependency tracking 밖에서 바뀌기 때문).
- **Equipment/Buff:** Infinite GE + Additive. 장착 시 Apply, 탈착 시 Remove. Override 방식 사용 금지 (Secondary GE와 충돌).
- **`RecalculateSecondaryAttributes`가 필요한 시점:** 레벨업 (`AddToPlayerLevel_Implementation`), 맵 이동 복원 후 (PossessedBy에서 travel 시 ApplyEffect로 처리).

### 맵 이동 데이터 유지 패턴
- **`UD1GameInstance` TMap 패턴:** Travel 직전 `SavePlayerData(PlayerId, FD1SavedPlayerData)` → 스폰 후 `TryGetPlayerData` + `ClearPlayerData`.
- **Travel 감지:** `GI->HasSavedData(GetPartyPlayerId())` 사용. `bAbilitySystemInitialized` 사용 금지 (non-seamless travel 시 PlayerState 재생성으로 항상 false → 초기화 중복 실행 버그).
- **PossessedBy 분기:** travel 아닐 때만 `InitializeDefaultAttributes()` 호출. travel일 때는 Secondary/Vital GE만 Apply 후 `RestoreTravelDataIfNeeded()`.
- **ServerTravel URL:** 짧은 맵 이름(`GoblinCave`)은 PIE에서만 작동. 패키징 빌드에서는 `/Game/Maps/GoblinCave` 전체 경로 필수. `D1GameModeTown::StartDungeonForParty`에서 `/` 없으면 자동 prefix 처리 중.

### 코드 패턴 (이 프로젝트 고유 규칙)
- **Server RPC는 반드시 `public:` 선언** — WidgetController 등 외부에서 호출. `private:`면 C2248 에러.
- **WidgetController 신규 추가 시 3곳 동시 수정:**
  1. `UD1WidgetController` 상속 클래스 신규 생성 (`.h/.cpp`)
  2. `D1HUD.h/.cpp` — getter, `TObjectPtr<UXxx>` 멤버, `TSubclassOf<UXxx>` Class 변수
  3. `D1AbilitySystemLibrary.h/.cpp` — `UFUNCTION(BlueprintPure)` static getter
- WidgetController 초기화 순서: `SetWidgetControllerParams` → `BindCallbacksToDependencies` → `BroadcastInitialValues`.
- 리프 표시 위젯(목록 아이템 등)은 WidgetController 연결 없이 `SetXxx(Struct)` 직접 주입. 상호작용은 이벤트 디스패처로 부모에 버블업.
- **변수명 shadowing 주의:** `APlayerState::PlayerId`가 `int32`로 존재 → 로컬 변수명 `PlayerId` 사용 금지 (C4458). `PartyId`, `CharacterId` 등으로 구분.

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
| 7 | 웹서버 연동 (로그인/영속화) | 🔄 진행중 (단일 프로세스 풀 루프 완료, 크로스 프로세스 travel 미완) |
| 8 | 마을/던전 분리 (ClientTravel + GameInstance 임시 저장) | ✅ 완료 |
| 9 | 어빌리티 추가 (ChargeDash, Focus, Heal) | ✅ 완료 |
| 10 | 던전 전투 루프 (몬스터/보스/클리어) | ✅ 완료 (몬스터 AI BT+EQS, 보스 사망→결과창→복귀) |
| 11 | 마을 파티 시스템 (생성/참가/던전 입장) | ✅ 완료 |
| 12 | Windows 데디서버 패키징 빌드 | ✅ 완료 |
| 13 | DB 스키마 + Spring Boot 백엔드 | ✅ 완료 (단일 서버 풀 루프 검증) |
| 14 | 크로스 프로세스 travel (DB 핸드오프) | 🔄 **현재 작업** |

### Phase 3 확정 일정 (2026-06-18 수정, 목표 **7/10 로컬 배포**, 주 5~6일 기준)

| 기간 | 마일스톤 |
|------|---------|
| ✅ ~6/12 | **언리얼 마무리**: 파티 풀 루프 2인 테스트 완료, Windows 데디서버 빌드 완료 |
| ✅ ~6/18 | DB 스키마 + Spring Boot + JWT + verify-session/save **단일 서버 풀 루프 완료**. GCP → Phase 4 이월, MVP는 **로컬 홈서버** |
| ~6/25 | **크로스 프로세스 travel**: Town:7777 → Dungeon:7778 ClientTravel (DB 핸드오프) + 던전→마을 복귀. Spring Boot `POST /api/server/sessions/issue` 신설 |
| ~7/1 | **보상 시스템**: 아이템 드롭→인벤토리→save 훅. ※ 몬스터 AI(BT+EQS)는 이미 완료 |
| ~7/7 | **통합 테스트 + 버그픽스**: 로그인→Town→파티→Dungeon→클리어→복귀→재로그인 데이터 유지 전체 루프 |
| ~7/10 | 데모 영상 촬영 |

**Scope 컷 (7/10 일정 성립 조건):**
- 🔪 **GCP/클라우드 배포 → Phase 4 이월.** MVP는 로컬 홈서버(i5-14400F + RAM 64GB, 포트포워딩)
- 🔪 **ECS/Docker 동적 오케스트레이션 → Phase 4 이월.** MVP는 고정 프로세스 2개(Town:7777, Dungeon:7778)
- 🔪 캐릭터 생성 화면 최소화 (커스터마이징 없이 계정당 1캐릭터 자동 생성)
- 🔪 Redis 없이 MySQL 직접 write (MVP 동접 수십 명 수준에서 충분)
- ~~몬스터 AI~~ **완료** (BT+EQS 근접/원거리 구현됨). 보스 복잡 패턴은 Phase 4
- **플랜 B** (7/1 마일스톤 지연 시): 던전 콘텐츠 축소(보스 1종만), 통합 테스트 우선
- **지켜야 할 척추**: 로그인(JWT) → DB 로드 → Town 접속 → 파티 → Dungeon 크로스 이동(DB 핸드오프) → 클리어 → 복귀 → 재로그인 시 데이터 유지

---

## 4. 완료: 11번 파티 시스템 + 12번 데디서버 빌드

### 파티 시스템 C++ (완료)
- **플레이어 식별: `GetPartyPlayerId()`.** Phase 3에서 웹서버 캐릭터 ID로 이 함수만 교체.
- `FD1PartyInfo`, `FD1PartyMemberInfo`, `AD1GameStateTown`, `AD1GameModeTown`, `AD1PlayerController` Server RPC, `UD1DungeonPartyWidgetController` 구현 완료.
- 던전 맵 화이트리스트 (`AllowedDungeonMaps`), PartyName 30자 제한, MaxMembers 검증 완료.
- **파티 풀 루프 2인 테스트 완료** (생성→가입→던전→보스→결과창→마을 복귀).

### 데디서버 빌드 (완료)
- `D1Server.Target.cs` 추가 (`Source/` 폴더).
- 소스 엔진 전환 (`D:\UnrealEngineSource\UnrealEngine`).
- UAT 배치파일 일체 작성 (`D:\D1\BuildAll.bat` 등).
- 패키징 빌드 서버/클라 모두 정상 동작 확인.

---

## 5. 진행중: 10번 던전 전투 루프

- **완료:** 보스 사망 감지 → 결과창 RPC → "Return to Town" 버튼.
- **미완:** 몬스터 AI, 보상 시스템.
- 관련 파일: `D1GameModeDungeon.h/.cpp`, `D1DungeonResultWidgetController.h/.cpp`, `D1Enemy.h/.cpp`

---

## 6. 완료된 시스템 요약

### 인벤토리 (6번, Phase 1 완료)
- `UD1ItemData` (PrimaryDataAsset), `FD1InventoryItem` (슬롯), `UD1InventoryComponent` (PlayerState 부착)
- 소비 아이템 GE 연동, 장비 장착/탈착 (Infinite GE Additive Apply/Remove), 탈착 시 인벤토리 자동 반환
- 멀티플레이어 2인 테스트 완료. 데이터는 메모리 (Phase 3에서 DB 영속화)

### 마을/던전 분리 (8번 완료)
- `UD1GameInstance` TMap 패턴으로 Travel 시 데이터 보존.
- Travel 감지: `GI->HasSavedData(GetPartyPlayerId())` (bAbilitySystemInitialized 사용 금지).
- 한계: 같은 프로세스 내에서만 유효 → Phase 3에서 DB 핸드오프로 교체.

### GAS Secondary 확정 구조 (완료)
- Primary: Infinite+Additive, Secondary: Infinite+Additive (MMC), Equipment/Buff: Infinite+Additive.
- `RefreshEquipEffects()` 제거 완료 (불필요).
- 레벨업 시 `RecalculateSecondaryAttributes()` 호출 (`AddToPlayerLevel_Implementation`).

### 어빌리티 (9번 완료)
- 근접 기본 공격, 광역 스킬, ChargeDash, Focus, Heal.
- 패시브 4종 예정: 불굴의 의지 / 마나 재생 / 강인함 / 전투 본능.

---

## 7. 서버 아키텍처 설계 (Phase 3)

### 통신 원칙
- `[Client] ←RPC→ [Dedicated Server] ←HTTP/JSON→ [Spring Boot] ←→ [MySQL]`
- 클라이언트는 웹서버에 게임 패킷 절대 직접 전송 안 함 (로그인/매칭만 HTTPS)
- 영속 데이터(레벨, XP, 인벤토리)는 로드/세이브 시점 동기화

### 기술 스택
- **백엔드:** Spring Boot (Kotlin 권장), MySQL (Redis 없이 MVP 시작)
- **인프라:** GCP (Cloud SQL for MySQL, GCE 또는 Cloud Run)
- **인증:** JWT

### 접속 플로우
1. `POST /login` → JWT 발급 → `GET /characters` → `POST /matchmaking/town`
2. 웹서버가 IP/Port/SessionToken(1회용) 응답 → 클라이언트 `ClientTravel` 직접 접속
3. 데디서버 → 웹서버 `POST /verify-session` (별도 API Key 인증) → DB 로드 → GAS 초기화

### MVP 서버 구성 (Phase 3 확정)
- **로컬 홈서버**: Town(7777) + Dungeon(7778) 프로세스 2개 고정 기동, 포트포워딩으로 외부 접속
- 동적 오케스트레이션(ECS/Docker), GCP 클라우드 배포 → Phase 4

### 크로스 프로세스 travel 설계 (Phase 3 핵심 잔여 작업)
- Spring Boot `POST /api/server/sessions/issue` (API Key, CharacterId → 60초 세션토큰) 신설
- Town GM: 파티원 전원 `SaveCharacter` → `IssueSessionToken` 각각 → `ClientTravel("dungeon_ip:7778?sessionToken=...")`
- Dungeon server: 기존 `verify-session` 재사용 (DB 로드 경로 그대로)
- 던전 클리어 시: Dungeon GM → 파티원 전원 `SaveCharacter` → `IssueSessionToken` → `ClientTravel("town_ip:7777?sessionToken=...")`
- Town server: 기존 `verify-session` 재사용 (DB 로드)

---

## 8. 작업 이력

| 날짜 | 요약 |
|------|------|
| 05-02~03 | 초기 셋업, GameDataReference.md, 아이템 데이터 구조 기획 |
| 05-04~08 | 인벤토리 WidgetController/UI/사용/장비 장착·탈착 |
| 05-11 | 전체 로드맵 + 서버 아키텍처 설계 확정 |
| 05-12 | 인벤토리 Phase 1 완료, 멀티 2인 테스트. 어빌리티 기획 |
| 05-20 | ChargeDash 완성 (DashToLocation AbilityTask) |
| 05-25 | Focus/Heal 완성, Secondary Init GE Instant 변경, RecalculateSecondaryAttributes |
| 05-26 | ClientTravel PlayerState 리셋 해결 (GameInstance 패턴), 던전 결과창, 보스 사망 감지 |
| 06-09~10 | 파티 시스템 C++ + UI 구현 완료. PlayerId 추상화(`GetPartyPlayerId()`). 코드 품질 개선(null 가드, 로그 카테고리, 서버 검증). Phase 3 일정 확정. |
| 06-13~14 | GAS 구조 확정 (Primary/Secondary/Equipment 모두 Infinite+Additive). Travel 감지 버그 수정 (HasSavedData). `RefreshEquipEffects` 제거. D1Server.Target.cs 추가. 소스 엔진 전환. 배치파일 일체 작성. 패키징 빌드 완료. 파티 풀 루프 2인 테스트 통과. |
| 06-14 | Phase 3 시작 — DB 스키마 + Spring Boot + GCP 배포 |
| 06-15 | HTTP 모듈 추가 (D1.Build.cs: HTTP/Json/JsonUtilities). 로그인 전용 사전화면 C++ 구현: `UD1HttpSubsystem`(GameInstanceSubsystem, JWT/AccountId/CharacterId 보관, Login/GetCharacters/CreateCharacter), `UD1LoginWidgetController`, `UD1CharacterSelectWidgetController`, `AD1PreGameHUD`(로그인↔캐릭터선택 위젯 전환). GAS 없는 사전화면이므로 UD1WidgetController 미상속. |
| 06-15 | 로그인 흐름 수정: 로그인 성공 후 GetCharacters() 먼저 호출 → 응답 도착 후 OnLoginSuccess 브로드캐스트 → 캐릭터 선택창 전환. 비동기 순서 보장으로 목록 없는 상태로 UI 열리는 문제 해결. `D1PreGameHUD::ShowCharacterSelect()`의 중복 RequestGetCharacters() 제거. 언리얼 클라 → 웹서버 로그인 테스트 통과. |
| 06-16 | PreGameHUD BeginPlay에 마우스 커서 표시 + UI Only 입력모드 추가. `FD1CharacterInfo`에 Level 필드 추가(파싱 포함). 데디서버 접속 플로우 클라 구현: `UD1HttpSubsystem::EnterTown(CharacterId)`(POST /api/matchmaking/town → 응답의 serverAddress+sessionToken으로 `ClientTravel("addr?sessionToken=...")`), `UD1CharacterSelectWidgetController::RequestEnterTown` (기존 SelectCharacterAndConnect/OpenLevel 교체). 세션 토큰은 BP 미노출, 실패 시에만 OnEnterTownFailed 브로드캐스트. |
| 06-18 | ③ 데디서버 DB 로드 구현. 서버용 HTTP: `UD1HttpSubsystem::VerifySession(token, 콜백)` (X-Server-Api-Key, 요청별 람다로 플레이어별 라우팅, `FD1LoadedStats` 파싱). 토큰 캡처: `AD1GameModeBase::InitNewPlayer`에서 `ParseOption(sessionToken)` → PlayerState(`PendingSessionToken`/`WebCharacterId`). GAS 적용: PossessedBy에 DB로그인 early-branch `AD1Hero::InitializeFromDbLogin` 분리 — 동기로 Secondary/Vital+어빌리티만, Primary/Level은 verify-session 콜백에서 `AD1PlayerState::ApplyLoadedStats`(기존 캐릭터) 또는 `InitializeDefaultAttributes`(신규)로 적용 후 RecalculateSecondary+UpdateAbilityStatuses. 기존 PIE/travel 경로는 그대로(이중 Primary 적용 방지 위해 DB로그인은 InitializeDefaultAttributes 미호출). STR=99 저장→접속→게임 내 반영 검증 완료. |
| 06-18 | PossessedBy 리팩토링 — 3개 접속 경로를 동일 층위 함수로 분리: `InitializeFreshSpawn`(직업 기본값) / `InitializeFromTravel`(GameInstance 복원) / `InitializeFromDbLogin`(DB 비동기 로드). PossessedBy는 `bDbLogin / bTraveling / else` 디스패처로 단순화. fresh·travel 공유 꼬리는 `FinalizeGAS()`(RecalcSecondary+AddCharacterAbilities+UpdateAbilityStatuses+RestoreAbilityStates)로 추출. 접속 시 체력/마나 풀충전 `RefillVitals()` 추가(DB로그인 콜백 끝). 호출 순서 보존(위치만 이동). |
| 06-18 | ③-b 인벤토리/장비 DB 로드. verify-session 응답에 inventory/equippedItems 파싱 추가 → `FD1LoadedCharacter`(Stats+Inventory+Equipped) 컨테이너로 델리게이트 전달. `AD1PlayerState::ApplyLoadedInventory` — DB 타입(item_asset_id 문자열/slot_type 문자열)을 게임 타입(FName/EEquipmentSlot, `StaticEnum` 변환)으로 바꿔 기존 `RestoreFromSave` 재사용(장비 GE 재적용 포함). DB로그인 콜백 순서: Stats → 인벤토리/장비 → RecalcSecondary → UpdateStatuses → RefillVitals. **`D1PlayerState::BeginPlay`의 테스트 아이템 하드코딩(Potion/Sword) 제거** — 인벤토리는 이제 DB가 단일 소스. |
| 06-18 | ③-c 스킬/스킬슬롯 DB 로드. verify-session에 skills/skillSlots 파싱 추가(`FD1LoadedSkill`/`FD1LoadedSkillSlot`). `AD1PlayerState::ApplyLoadedSkills` — DB 스킬을 `FD1SavedAbilityInfo`로 변환(skill_tag→`RequestGameplayTag`, slot_key Q/W/E/R→`InputTag_*`, 슬롯에 있으면 Equipped+SlotTag 없으면 Unlocked) 후 기존 `RestoreAbilityStates` 재사용. 콜백에서 **UpdateAbilityStatuses 이후** 호출(Eligible→Unlocked/Equipped 덮어쓰기). ⚠️ **아이템 퀵슬롯(character_quick_slots, 1~4)은 게임에 시스템 미구현이라 로드 제외** — 추후 아이템 퀵슬롯 기능 추가 시 반영. ※ 로컬 변수명 `Tags`는 `AActor::Tags`와 충돌(C4458) → `GameTags`로. |
| 06-18 | ③-d 저장(save) — 로그아웃 시점. `UD1HttpSubsystem::SaveCharacter(id, jsonBody)`(POST /save, API Key, fire-and-forget — 본문 동기 복사라 PS 소멸 무관). `AD1PlayerState::BuildSaveJson()` — ApplyLoaded*의 역방향 직렬화(PS/AS getter + `SaveAbilityStates()` + 인벤토리 getter → DB 계약 JSON; InputTag→슬롯키, EEquipmentSlot→짧은이름, quickSlots 빈배열). `AD1GameModeBase::Logout`에서 `WebCharacterId>0`이면 저장 — 정상 종료/강제 킬(연결 타임아웃) 둘 다 Logout으로 진입. 서버 크래시만 미커버(추후 주기 autosave). 풀 루프(로드→플레이→Logout 저장→재접속 유지) 검증 완료. travel/던전클리어 저장은 같은 함수 재사용 예정. |
| 06-18 | `SaveAbilityStates` 버그픽스 — Locked만 제외하던 것을 **Locked+Eligible 둘 다 제외**로 변경. Eligible(레벨로 자동 결정되는 미습득 상태)이 DB/travel에 저장되면 "안 배웠는데 배운 걸로" 의미 오염 → Unlocked/Equipped(실제 습득)만 저장. travel·DB 저장 양쪽에 일관 적용. |
| 06-18 | **레벨업 알림 오발동 버그픽스.** 접속/Travel 시 DB 복원(`ApplyLoadedStats`)이 같은 `OnLevelChangedDelegate`를 터뜨려 BP에서 "레벨업!" 알림이 표시되던 문제. `AD1PlayerState::AddToLevel()`에만 `FOnPlayerActualLevelUp OnActualLevelUpDelegate` 추가 (XP 획득 경로 전용). `UD1OverlayWidgetController`에 `FOnPlayerLeveledUpSignature OnPlayerLeveledUpDelegate` (BlueprintAssignable) 신설하여 바인딩. BP에서 레벨업 알림을 `OnPlayerLevelChangedDelegate` → `OnPlayerLeveledUpDelegate`로 교체하면 복원/동기화 시 오발동 없어짐. |
| 06-18 | **일정 개정** — GCP 배포 Phase 4 이월, MVP를 로컬 홈서버(포트포워딩)로 확정. 주 5~6일 기준으로 목표를 6/30 → **7/10**으로 조정. 크로스 프로세스 travel(DB 핸드오프)이 다음 주요 작업. |
