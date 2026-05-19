# 어빌리티 추가 개발 기획서

## 작성일: 2026-05-12
## 목적: 인벤토리 Phase 1 완료 후, 핵심 전투 루프를 완성하기 위한 3개 어빌리티 기획

---

## 1. 대시/회피 (Dash / Evasion)

### 개요
- **이름:** Dash
- **입력:** Enhanced Input Action `IA_Dash` (Space 또는 Shift)
- **목적:** 기동성 부여, 적 공격 회피

### C++ 클래스
- `UGameplayAbility` 상속: `UD1Ability_Dash`
- **파일:** `Source/D1/AbilitySystem/Abilities/D1Ability_Dash.h`, `.cpp`

### 동작 흐름
```
[ActivateAbility]
  → 서버 권한 확인 (HasAuthority)
  → Ability Cost 확인 (Mana 확인, 필요 시)
  → Cooldown GE 적용 (Abilities.Dash.Cooldown)
  → RootMotion Dash 실행
    - Ability Task: UAbilityTask_ApplyRootMotionConstantForce
    - 방향: 현재 캐릭터 정면 (Forward Vector)
    - 거리: 300~500 units
    - 시간: 0.2초
  → 무적 GE 적용 (Duration: 0.3초)
    - GameplayTag: State.Invincible
    - Effect: IncomingDamage = 0 (Modifier)
  → GameplayCue: Dash 이펙트 (Niagara)
  → AnimInstance: Dash 몽타주 재생
  → Ability End
```

### GameplayEffect
| GE 이름 | Duration | Effect | Tags |
|---------|----------|--------|------|
| `GE_Cooldown_Dash` | Duration (5초) | Cooldown | Abilities.Dash.Cooldown |
| `GE_Invincible_Dash` | Duration (0.3초) | IncomingDamage = 0 | State.Invincible |

### GameplayTags
- `Abilities.Dash`: 어빌리티 태그
- `Abilities.Dash.Cooldown`: 쿨다운 태그
- `State.Invincible`: 무적 상태 태그

### 주의사항
- **멀티플레이어:** RootMotion은 서버에서 계산, 클라이언트는 Replication으로 위치 동기화
- **지형 체크:** Dash 도착 지점이 벽/장애물이면 Ability 캔슬 또는 거리 조정 (LineTrace)
- **연속 입력 방지:** Cooldown Tag가 있으면 ActivateAbility가 막힘 (GAS 기본 동작)

---

## 2. 자기 힐/보호막 (Self Heal / Shield)

### 개요
- **이름:** Heal
- **입력:** Enhanced Input Action `IA_Heal` (예: Q)
- **목적:** 생존력 확보, 포션 외 추가 회복 수단

### C++ 클래스
- `UGameplayAbility` 상속: `UD1Ability_Heal`
- **파일:** `Source/D1/AbilitySystem/Abilities/D1Ability_Heal.h`, `.cpp`

### 동작 흐름
```
[ActivateAbility]
  → 서버 권한 확인
  → Ability Cost 확인 (Mana: MaxMana * 0.25)
  → Cooldown GE 적용 (Abilities.Heal.Cooldown)
  → Instant GE: Health 회복
    - Attribute: Health
    - Modifier: Add (회복량)
    - Clamp: MaxHealth 이상 불가 (AttributeSet PreAttributeChange에서 처리)
  → GameplayCue: 힐 이펙트 (Niagara + 사운드)
  → AnimInstance: 힐 시전 몽타주 (简단한 손동작)
  → Ability End
```

### GameplayEffect
| GE 이름 | Duration | Effect | Tags |
|---------|----------|--------|------|
| `GE_Cooldown_Heal` | Duration (10초) | Cooldown | Abilities.Heal.Cooldown |
| `GE_Heal` | Instant | Health += HealAmount | (없음) |

### DataAsset 설정
- `HealAmount`: 기본 30, Level Scaling 가능 (ScalableFloat)
- `Cost`: Mana (MaxMana의 25%)

### 주의사항
- **Overheal 방지:** `UD1AttributeSet::PreAttributeChange`에서 Health <= MaxHealth로 Clamp
- **멀티플레이어:** Health는 `ReplicatedUsing = OnRep_Health`로 동기화됨
- **GameplayCue:** 클라이언트에서만 이펙트 표시 (서버 부하 감소)

---

## 3. 적 넉백/스턴 (Knockback / Stun)

### 개요
- **이름:** StunStrike
- **입력:** Enhanced Input Action `IA_Stun` (예: E)
- **목적:** CC (Crowd Control), 몬스터 행동 제한

### C++ 클래스
- `UGameplayAbility` 상속: `UD1Ability_StunStrike`
- **파일:** `Source/D1/AbilitySystem/Abilities/D1Ability_StunStrike.h`, `.cpp`

### 동작 흐름
```
[ActivateAbility]
  → 서버 권한 확인
  → Ability Cost 확인 (Mana: 중간)
  → Cooldown GE 적용 (Abilities.Stun.Cooldown)
  → 전방 SphereOverlap (또는 LineTrace)
    - 중심: 캐릭터 위치 + Forward * 100
    - 반경: 150 units
    - 필터: 적 팀 (Enemy Tag)
  → 대상마다 Loop:
    - Instant GE: Damage (AttackPower 기반)
    - Duration GE: Stun (2초)
      - GameplayTag: State.Stunned
      - Effect: CharacterMovementComponent MovementMode = None
      - Effect: Ability 입력 차단 (Abilities.Block)
    - Physics Impulse 적용 (넉백)
      - LaunchCharacter 또는 AddImpulse
      - 방향: 캐릭터 → 적 방향 (Normalized)
      - 세기: 500
  → GameplayCue: 충격파 이펙트 (Niagara)
  → AnimInstance: 전방 가격 몽타주
  → Ability End
```

### GameplayEffect
| GE 이름 | Duration | Effect | Tags |
|---------|----------|--------|------|
| `GE_Cooldown_Stun` | Duration (8초) | Cooldown | Abilities.Stun.Cooldown |
| `GE_Stun` | Duration (2초) | Movement blocked, Ability blocked | State.Stunned, Abilities.Block |
| `GE_Damage_StunStrike` | Instant | Health -= Damage | (없음) |

### GameplayTags
- `Abilities.Stun`: 어빌리티 태그
- `Abilities.Stun.Cooldown`: 쿨다운 태그
- `State.Stunned`: 스턴 상태 태그
- `Abilities.Block`: Ability 입력 차단 태그

### 주의사항
- **멀티플레이어:** `LaunchCharacter`는 서버에서 호출, 클라이언트는 Replication으로 위치/속도 동기화
- **Stun 해제:** `GE_Stun` Duration이 끝나면 자동 해제 (GAS 기본). 태그 제거 시 이동/Ability 복구
- **보스 예외:** 보스 몬스터는 `State.BossImmunity` 태그가 있으면 Stun 무효 (GE GrantedTags로 조건부 적용)
- **넉백 방향:** 캐릭터가 적을 향해 공격할 때, 적이 캐릭터 뒤로 날아가야 함 (Vector: Target - Source, Normalized)

---

## 공통 구현 체크리스트

### Enhanced Input
- [ ] `IA_Dash`, `IA_Heal`, `IA_Stun` Input Action Asset 생성
- [ ] `IMC_Player` Input Mapping Context에 추가
- [ ] `D1Hero`에서 Enhanced Input Bind (SetupPlayerInputComponent 또는 BeginPlay)

### GameplayAbilityDataAsset
- [ ] `DA_Ability_Dash`, `DA_Ability_Heal`, `DA_Ability_Stun` 생성
  - Ability Class: 각 C++ Ability 클래스
  - Cooldown GE: 위에 정의한 Cooldown GE
  - Cost GE: Mana Cost GE (Heal/Stun만, Dash는 없음 또는 소량)
  - Tags: Ability.AbilityName, Ability.AbilityName.Cooldown

### GameplayTags
- [ ] `D1GameplayTags.h/.cpp`에 아래 태그 추가:
  - `Abilities.Dash`
  - `Abilities.Dash.Cooldown`
  - `Abilities.Heal`
  - `Abilities.Heal.Cooldown`
  - `Abilities.Stun`
  - `Abilities.Stun.Cooldown`
  - `State.Invincible`
  - `State.Stunned`
  - `Abilities.Block`

### AnimInstance
- [ ] Dash 몽타주 (간단한 회피 동작)
- [ ] Heal 몽타주 (손 모으기 또는 포션 마시기)
- [ ] StunStrike 몽타주 (전방 강격)

### Niagara / GameplayCue
- [ ] Dash 잔상 이펙트
- [ ] Heal 회복 빛 이펙트
- [ ] StunStrike 충격파 이펙트

### HUD / UI
- [ ] `WBP_SkillMenu`에 3개 아이콘 추가
- [ ] 각 아이콘에 쿨타임 오버레이 (`AbilitySystemComponent::GetActiveEffectsTimeRemainingAndDuration`)
- [ ] 입력 키 표시 (Space, Q, E)

---

## GAS 학습 포인트

| 어빌리티 | 학습 포인트 |
|----------|------------|
| **Dash** | `UAbilityTask_ApplyRootMotionConstantForce`, Duration GE, 무적 태그 |
| **Heal** | Instant GE, Attribute Clamp, GameplayCue, Cost (ScalableFloat) |
| **StunStrike** | SphereOverlap, 다중 타겟 GE 적용, Physics Impulse, 태그 기반 행동 차단 |

---

## 다음 단계 (작업 순서)

1. **GameplyTags 추가** (`D1GameplayTags.h/.cpp`)
2. **Enhanced Input Action/Mapping** 생성
3. **C++ Ability 클래스** 생성 (Dash → Heal → StunStrike 순)
4. **GameplayEffect Blueprint** 생성 (Cooldown, Invincible, Heal, Stun)
5. **AnimInstance 몽타주** 연결
6. **GameplayCue** 설정
7. **HUD 아이콘** 추가 및 쿨타임 연동
8. **PIE 테스트** (서버/클라이언트 각각)
9. **멀티플레이어 테스트** (2인 이상)
