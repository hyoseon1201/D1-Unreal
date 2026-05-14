# 작업 로그: 2026-05-11 (2차)

## 작업 내용

### 1. 클라-서버 통신 아키텍처 확정
- **핵심 원칙:** 클라이언트는 게임 패킷을 웹서버에 별내지 않음. 로그인/매칭만 웹서버(HTTPS) 처리.
- **실시간 게임 통신:** 클라이언트 ↔ Dedicated Server가 UE5 Replication/RPC로 직접 통신
- **전체 유저 플로우:**
  1. `POST /login` → JWT AccessToken 발급
  2. `GET /characters` → 캐릭터 목록 (Token 헤더 포함)
  3. `POST /matchmaking/town` → 웹서버가 현재 마을 서버 목록 확인 후 IP:Port + SessionToken 응답
  4. 클라이언트 `ClientTravel(IP:Port)` → Dedicated Server 직접 접속
  5. 데디 서버 `POST /verify-session` (Token 검증)
  6. 검증 OK → DB에서 캐릭터 데이터 로드 → PlayerState 생성 → GAS AttributeSet 초기화

### 2. 마을 서버 vs 던전 서버 스케일링 문제 해결
- **문제:** 마을 100명 vs 던전 4인이 똑같은 UE5 서버 바이너리를 쓰면 리소스 낭비
- **해결책 1 — 채널(샤드) 분리:**
  - 마을도 1서버당 50~100명만 처리 (Lost Ark/MapleStory 방식)
  - 유저가 채널을 직접 선택하거나 자동 배정
- **해결책 2 — 서버 모드 분리 (커맨드라인 인자):**
  - `D1Server.exe -TownServer -MaxPlayers=100 -AISpawn=0`
  - `D1Server.exe -DungeonServer -MaxPlayers=4 -Level=DG_Forest_01`
  - C++에서 `FParse::Param`으로 모드 확인 후 기능 ON/OFF
    - 마을: AI 스폰 최소화, Physics Tick 낮춤, 전투 로직 비활성화
    - 던전: 풀 전투, 몬스터 AI 활성화, 60Hz Tick
- **해결책 3 — K8s 리소스 할량 차등:**
  - 마을 서버 Pod: 4Gi 메모리 / 2 vCPU
  - 던전 서버 Pod: 1.5Gi 메모리 / 0.5 vCPU
  - 던전은 Stateless: HPA로 입장 시 생성, 퇴장 시 자동 파괴
- **해결책 4 — 서버 전용 Cook:**
  - `D1Server.Target.cs`에서 개발자 도구/에디터 데이터 제거
  - 텍스처/머티리얼/사운드 제거된 서버 전용 Cook으로 바이너리 및 메모리 최적화

### 3. 보안/성능 주의사항
- 웹서버가 게임 패킷을 프록시하면 안 됨 (HTTP 요청-응답 구조는 실시간 게임에 부적합, UDP 기반 직접 통신 필요)
- SessionToken은 1회용, 데디 서버 인증 후 즉시 폐기 (재사용 공격 방지)
- 데디 서버 → 웹서버 통신 시 낮 API Key로 인증, 외부에 HTTP 엔드포인트 노출 금지

## 비고
- `CONTEXT.md`의 "7~8번 상세" 섹션에 서버 아키텍처 설계를 별도로 정리하여 추가함
- Phase 2 (웹서버 연동) 설계가 이론적으로 확정됨. 실제 구현은 인벤토리 Phase 1 완료 후 진행 예정
- 던전 입장 시 `SeamlessTravel` (UE5 서버 트래블) 방식 적용 검토 필요
