// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1Hero.h"

#include "D1/D1.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/D1PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "UI/HUD/D1HUD.h"
#include "Player/D1PlayerController.h"
#include "NiagaraComponent.h"
#include "AbilitySystem/Data/D1LevelupInfo.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "Game/D1GameInstance.h"

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

	// Init ability actor info for the Server
	// 맵 이동(ClientTravel) 후에도 새 Pawn의 ASC 참조를 갱신해야 하므로 항상 호출
	InitAbilityActorInfo();

	if (AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>())
	{
		// 1. Attribute 초기화
		// Travel 여부는 GameInstance 저장 데이터 유무로 판단한다.
		// (bAbilitySystemInitialized는 PlayerState가 매번 재생성되므로 항상 false — 사용 불가)
		const UD1GameInstance* GI = Cast<UD1GameInstance>(GetGameInstance());
		const bool bIsTraveling = GI && GI->HasSavedData(D1PS->GetPartyPlayerId());

		if (!bIsTraveling)
		{
			// 최초 스폰: Primary + Secondary + Vital 전부 초기화
			InitializeDefaultAttributes();
			UE_LOG(LogD1Travel, Verbose, TEXT("PossessedBy: InitializeDefaultAttributes executed. Level [%d]"), D1PS->GetPlayerLevel());
		}
		else
		{
			// Travel: Secondary + Vital만 재적용 (Primary는 RestoreTravelDataIfNeeded에서 직접 Set)
			ApplyEffectToSelf(DefaultSecondaryAttributes, (float)D1PS->GetPlayerLevel());
			ApplyEffectToSelf(DefaultVitalAttributes, (float)D1PS->GetPlayerLevel());
			UE_LOG(LogD1Travel, Verbose, TEXT("PossessedBy: Travel — Secondary+Vital re-applied. Level [%d]"), D1PS->GetPlayerLevel());
		}

		D1PS->bAbilitySystemInitialized = true;

		// 2. GameInstance에서 저장된 데이터 복원 (Primary Attribute Override)
		// InitializeDefaultAttributes 이후에 실행하여 기본값을 덮어씀
		D1PS->RestoreTravelDataIfNeeded();

		// 3. Primary 복원 후 Secondary 재계산
		// RestoreTravelDataIfNeeded가 Strength 등을 덮어쓰므로, Instant Secondary GE를
		// 다시 적용해 BaseValue를 복원된 Primary 기준으로 갱신한다.
		UD1AbilitySystemLibrary::RecalculateSecondaryAttributes(this, D1PS->GetAbilitySystemComponent());

		// 3. Ability는 항상 재등록 (맵 이동 시 새 Pawn의 새 ASC에 필요)
		// AddCharacterAbilities 내부에서 중복 체크됨
		AddCharacterAbilities();

		// 4. Ability 상태(잠금/해금)는 레벨에 따라 변하므로 항상 업데이트
		if (UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(GetAbilitySystemComponent()))
		{
			D1ASC->UpdateAbilityStatuses(D1PS->GetPlayerLevel());

			// 5. Travel 복원: UpdateAbilityStatuses 완료 후 저장된 스킬 상태(Unlocked/Equipped/퀵슬롯) 적용
			if (!D1PS->PendingAbilityRestoreData.IsEmpty())
			{
				D1ASC->RestoreAbilityStates(D1PS->PendingAbilityRestoreData);
				D1PS->PendingAbilityRestoreData.Empty();
			}
		}
	}
	else
	{
		UE_LOG(LogD1Travel, Error, TEXT("PossessedBy: PlayerState NOT found!"));
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
