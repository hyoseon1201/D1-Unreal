// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1Hero.h"

#include "D1/D1.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/D1PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "UI/HUD/D1HUD.h"
#include "Player/D1PlayerController.h"
#include "NiagaraComponent.h"
#include "AbilitySystem/Data/D1LevelupInfo.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "Game/D1GameInstance.h"
#include "Game/D1HttpSubsystem.h"

AD1Hero::AD1Hero()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>("TopDownCameraComponent");
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	LevelUpNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("LevelUpNiagaraComponent");
	LevelUpNiagaraComponent->SetupAttachment(GetRootComponent());
	LevelUpNiagaraComponent->bAutoActivate = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
}

void AD1Hero::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 맵 이동(ClientTravel) 후에도 새 Pawn의 ASC 참조를 갱신해야 하므로 항상 호출
	InitAbilityActorInfo();

	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	if (!D1PS)
	{
		UE_LOG(LogD1Travel, Error, TEXT("PossessedBy: PlayerState NOT found!"));
		return;
	}

	// 접속 경로에 따라 GAS 초기화 방식이 갈린다.
	// (bAbilitySystemInitialized는 PlayerState가 매번 재생성되므로 판별에 사용 불가)
	const UD1GameInstance* GI = Cast<UD1GameInstance>(GetGameInstance());
	const bool bDbLogin   = HasAuthority() && !D1PS->PendingSessionToken.IsEmpty();
	const bool bTraveling = GI && GI->HasSavedData(D1PS->GetPartyPlayerId());

	if (bDbLogin)
	{
		InitializeFromDbLogin(D1PS);   // 웹서버 로그인 / 크로스 프로세스 접속 → DB 로드 (비동기)
	}
	else if (bTraveling)
	{
		InitializeFromTravel(D1PS);    // 같은 프로세스 맵 이동 → GameInstance 복원
	}
	else
	{
		InitializeFreshSpawn(D1PS);    // 최초 스폰 (PIE 등) → 직업 기본값
	}
}

void AD1Hero::InitializeFreshSpawn(AD1PlayerState* D1PS)
{
	// 최초 스폰: Primary + Secondary + Vital 전부 직업 기본값으로 초기화
	InitializeDefaultAttributes();
	D1PS->bAbilitySystemInitialized = true;

	FinalizeGAS(D1PS);
	UE_LOG(LogD1Travel, Verbose, TEXT("InitializeFreshSpawn: defaults applied. Level [%d]"), D1PS->GetPlayerLevel());
}

void AD1Hero::InitializeFromTravel(AD1PlayerState* D1PS)
{
	// 같은 프로세스 맵 이동: Secondary + Vital만 재적용 (Primary는 RestoreTravelDataIfNeeded에서 직접 Set)
	ApplyEffectToSelf(DefaultSecondaryAttributes, (float)D1PS->GetPlayerLevel());
	ApplyEffectToSelf(DefaultVitalAttributes, (float)D1PS->GetPlayerLevel());
	D1PS->bAbilitySystemInitialized = true;

	// GameInstance에 저장된 Primary/Level/인벤토리 복원
	D1PS->RestoreTravelDataIfNeeded();

	FinalizeGAS(D1PS);
	UE_LOG(LogD1Travel, Verbose, TEXT("InitializeFromTravel: restored. Level [%d]"), D1PS->GetPlayerLevel());
}

void AD1Hero::InitializeFromDbLogin(AD1PlayerState* D1PS)
{
	// 동기로는 Secondary/Vital baseline + 어빌리티만 처리.
	// Primary/Level은 verify-session 응답(비동기) 후 적용 — InitializeDefaultAttributes를
	// 동기 호출하지 않아 기본 Primary GE와 DB 값의 이중 적용을 방지한다.
	ApplyEffectToSelf(DefaultSecondaryAttributes, (float)D1PS->GetPlayerLevel());
	ApplyEffectToSelf(DefaultVitalAttributes, (float)D1PS->GetPlayerLevel());
	D1PS->bAbilitySystemInitialized = true;
	AddCharacterAbilities();

	UD1HttpSubsystem* Http = GetGameInstance()->GetSubsystem<UD1HttpSubsystem>();
	if (!Http)
	{
		UE_LOG(LogD1Travel, Error, TEXT("InitializeFromDbLogin: HttpSubsystem 없음 — 데이터 로드 불가"));
		return;
	}

	// 동시 접속 플레이어 간 결과 라우팅 + 응답 전 소멸 대비 약참조 캡처
	TWeakObjectPtr<AD1Hero> WeakSelf = this;
	TWeakObjectPtr<AD1PlayerState> WeakPS = D1PS;

	FD1VerifySessionDelegate OnVerify;
	OnVerify.BindLambda([WeakSelf, WeakPS](bool bSuccess, int64 CharacterId, const FD1LoadedCharacter& Data)
	{
		AD1Hero* Self = WeakSelf.Get();
		AD1PlayerState* PS = WeakPS.Get();
		if (!Self || !PS) return;   // 응답 전 Pawn/PS 소멸 (접속 종료 등)
		if (!bSuccess)
		{
			UE_LOG(LogD1Travel, Warning, TEXT("verify-session 실패 — 캐릭터 데이터 로드 안 됨"));
			return;
		}

		PS->WebCharacterId = CharacterId;

		// 1. Stats (Primary/Level)
		if (Data.Stats.bHasStats)
		{
			PS->ApplyLoadedStats(Data.Stats);   // 기존 캐릭터
			UE_LOG(LogD1Travel, Log, TEXT("DB 캐릭터 로드: CharId=%lld, Lv=%d STR=%.0f"),
				CharacterId, Data.Stats.Level, Data.Stats.Strength);
		}
		else
		{
			Self->InitializeDefaultAttributes();   // 신규 캐릭터: 직업 기본값 (저장은 ③-d)
			UE_LOG(LogD1Travel, Log, TEXT("신규 캐릭터(CharId=%lld) — 기본값 적용"), CharacterId);
		}

		// 2. 인벤토리/장비 (장비 GE 재적용 포함 → Secondary 재계산 전에 적용)
		PS->ApplyLoadedInventory(Data.Inventory, Data.Equipped);

		// 3. Primary/장비 확정 후: Secondary 재계산 → 레벨 기반 어빌리티 상태 갱신
		UD1AbilitySystemLibrary::RecalculateSecondaryAttributes(Self, PS->GetAbilitySystemComponent());
		if (UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(PS->GetAbilitySystemComponent()))
		{
			D1ASC->UpdateAbilityStatuses(PS->GetPlayerLevel());
		}

		// 4. 스킬 복원 (UpdateAbilityStatuses로 Eligible 세팅 후 저장된 Unlocked/Equipped로 덮어씀)
		PS->ApplyLoadedSkills(Data.Skills, Data.SkillSlots);

		// 5. 체력/마나 풀충전
		Self->RefillVitals();
	});

	Http->VerifySession(D1PS->PendingSessionToken, OnVerify);
}

void AD1Hero::FinalizeGAS(AD1PlayerState* D1PS)
{
	// Primary 확정 후 Secondary 재계산 (BaseValue를 확정된 Primary 기준으로 갱신)
	UD1AbilitySystemLibrary::RecalculateSecondaryAttributes(this, D1PS->GetAbilitySystemComponent());

	// Ability 재등록 (맵 이동 시 새 ASC에 필요, 내부 중복 체크) + 레벨 기반 상태 갱신
	AddCharacterAbilities();
	if (UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		D1ASC->UpdateAbilityStatuses(D1PS->GetPlayerLevel());

		// Travel 복원: 저장된 스킬 상태(Unlocked/Equipped/퀵슬롯) 적용
		if (!D1PS->PendingAbilityRestoreData.IsEmpty())
		{
			D1ASC->RestoreAbilityStates(D1PS->PendingAbilityRestoreData);
			D1PS->PendingAbilityRestoreData.Empty();
		}
	}
}

void AD1Hero::RefillVitals()
{
	// 접속/스폰 시 체력·마나를 최대치로 채운다 (MaxHealth/MaxMana 확정 이후 호출).
	if (UD1AttributeSet* AS = Cast<UD1AttributeSet>(AttributeSet))
	{
		AS->SetHealth(AS->GetMaxHealth());
		AS->SetMana(AS->GetMaxMana());
	}
}

void AD1Hero::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Init ability actor info for the Client
	InitAbilityActorInfo();
}

int32 AD1Hero::GetPlayerLevel_Implementation()
{
	const AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	return D1PS->GetPlayerLevel();
}

void AD1Hero::AddToXP_Implementation(int32 InXP)
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	D1PS->AddToXP(InXP);
}

int32 AD1Hero::GetXP_Implementation() const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	return D1PS->GetXP();
}

void AD1Hero::LevelUp_Implementation()
{
	MulticastLevelupParticles();
}

int32 AD1Hero::FindLevelForXP_Implementation(int32 InXP) const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	// LevelUpInfo 데이터 에셋 미할당 시 현재 레벨 유지 (레벨업 없음)
	if (!D1PS->LevelUpInfo)
	{
		UE_LOG(LogD1Ability, Error, TEXT("FindLevelForXP: LevelUpInfo is NULL! Assign the DataAsset in BP_D1PlayerState."));
		return D1PS->GetPlayerLevel();
	}
	return D1PS->LevelUpInfo->FindLevelForXP(InXP);
}

int32 AD1Hero::GetAttributePointsReward_Implementation(int32 Level) const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	// 레벨이 테이블 범위를 벗어나면(최대 레벨 초과, 외부 복원 값 등) 보상 0 처리
	if (!D1PS->LevelUpInfo || !D1PS->LevelUpInfo->LevelupInformation.IsValidIndex(Level))
	{
		UE_LOG(LogD1Ability, Warning, TEXT("AttributePointsReward: Level %d is out of LevelupInformation range. Returning 0."), Level);
		return 0;
	}
	return D1PS->LevelUpInfo->LevelupInformation[Level].AttributePointAward;
}

int32 AD1Hero::GetSkillPointsReward_Implementation(int32 Level) const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	// 레벨이 테이블 범위를 벗어나면(최대 레벨 초과, 외부 복원 값 등) 보상 0 처리
	if (!D1PS->LevelUpInfo || !D1PS->LevelUpInfo->LevelupInformation.IsValidIndex(Level))
	{
		UE_LOG(LogD1Ability, Warning, TEXT("SkillPointsReward: Level %d is out of LevelupInformation range. Returning 0."), Level);
		return 0;
	}
	return D1PS->LevelUpInfo->LevelupInformation[Level].SpellPointAward;
}

void AD1Hero::AddToPlayerLevel_Implementation(int32 InPlayerLevel)
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	D1PS->AddToLevel(InPlayerLevel);

	if (UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		D1ASC->UpdateAbilityStatuses(D1PS->GetPlayerLevel());
	}

	// 레벨 변경 후 Secondary GE를 새 레벨 기준으로 재적용
	// (Infinite GE는 생성 시점의 EffectLevel을 유지하므로 수동 갱신 필요)
	UD1AbilitySystemLibrary::RecalculateSecondaryAttributes(this, GetAbilitySystemComponent());
}

void AD1Hero::AddToSkillPoints_Implementation(int32 InSkillPoints)
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	D1PS->AddToSkillPoints(InSkillPoints);
}

void AD1Hero::AddToAttributePoints_Implementation(int32 InAttributePoints)
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	D1PS->AddToAttributePoints(InAttributePoints);
}

int32 AD1Hero::GetAttributePoints_Implementation() const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	return D1PS->GetAttributePoints();
}

int32 AD1Hero::GetSkillPoints_Implementation() const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	return D1PS->GetSkillPoints();
}

FGameplayTag AD1Hero::GetCharacterClassTag_Implementation() const
{
	return CharacterClassTag;
}

void AD1Hero::InitAbilityActorInfo()
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);

	D1PS->GetAbilitySystemComponent()->InitAbilityActorInfo(D1PS, this);
	Cast<UD1AbilitySystemComponent>(D1PS->GetAbilitySystemComponent())->AbilityActorInfoSet();
	AbilitySystemComponent = D1PS->GetAbilitySystemComponent();
	AttributeSet = D1PS->GetAttributeSet();

	if (AD1PlayerController* D1PlayerController = Cast<AD1PlayerController>(GetController()))
	{
		if (AD1HUD* D1HUD = Cast<AD1HUD>(D1PlayerController->GetHUD()))
		{
			D1HUD->InitOverlay(D1PlayerController, D1PS, AbilitySystemComponent, AttributeSet);
		}
	}
}

void AD1Hero::MulticastLevelupParticles_Implementation() const
{
	if (IsValid(LevelUpNiagaraComponent))
	{
		const FVector CameraLocation = TopDownCameraComponent->GetComponentLocation();
		const FVector NiagaraSystemLocation = LevelUpNiagaraComponent->GetComponentLocation();
		const FRotator ToCameraRotation = (CameraLocation - NiagaraSystemLocation).Rotation();
		LevelUpNiagaraComponent->SetWorldRotation(ToCameraRotation);
		LevelUpNiagaraComponent->Activate(true);
	}
}
