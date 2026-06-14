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
| 7 | 웹서버 연동 (로그인/영속화) | ⏳ Phase 3 |
| 8 | 마을/던전 분리 (ClientTravel + GameInstance 임시 저장) | ✅ 완료 |
| 9 | 어빌리티 추가 (ChargeDash, Focus, Heal) | ✅ 완료 |
| 10 | 던전 전투 루프 (몬스터/보스/클리어) | 🔄 진행중 |
| 11 | 마을 파티 시스템 (생성/참가/던전 입장) | ✅ 완료 |
| 12 | Windows 데디서버 패키징 빌드 | ✅ 완료 |
| 13 | DB 스키마 + Spring Boot 백엔드 | 🔄 **현재 작업** |

### Phase 3 확정 일정 (2026-06-14 기준, 목표 6/30)

| 기간 | 마일스톤 |
|------|---------|
| ✅ ~6/12 | **언리얼 마무리**: 파티 풀 루프 2인 테스트 완료, Windows 데디서버 빌드 완료 |
| ~6/16 | DB 스키마 (계정/캐릭터/스탯/인벤토리) + Spring Boot 셋업 + 로그인/JWT + **GCP 첫 배포** |
| ~6/21 | 캐릭터/스탯/인벤토리 저장·로드 API + UE HTTP 연동 (로그인 UI, verify-session) — ⚠️ **최대 리스크 구간** |
| ~6/26 | 크로스 서버 이동: GCP VM에 Town/Dungeon 프로세스 2개 고정 기동, GameInstance → DB 핸드오프 교체 |
| ~6/30 | 통합 테스트 + 버그픽스 + 데모 영상 촬영 |

**Scope 컷 (20일 일정 성립 조건):**
- 🔪 **ECS/Docker 동적 오케스트레이션 → Phase 4 이월.** MVP는 고정 프로세스 2개(Town:7777, Dungeon:7778)
- 🔪 캐릭터 생성 화면 최소화 (커스터마이징 없이 계정당 1캐릭터 자동 생성)
- 🔪 Redis 없이 MySQL 직접 write (MVP 동접 수십 명 수준에서 충분)
- **플랜 B** (6/21 마일스톤 지연 시): 크로스 서버 포기, 단일 서버 + DB 영속화까지만
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

### MVP 서버 구성
- GCP VM 1대에 Town(7777) + Dungeon(7778) 프로세스 고정 기동
- 동적 오케스트레이션(ECS/Docker)은 Phase 4

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
