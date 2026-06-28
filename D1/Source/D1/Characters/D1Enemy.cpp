// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1Enemy.h"

#include "D1/D1.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "Components/WidgetComponent.h"
#include "UI/Widget/D1UserWidget.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "AI/D1AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "D1GameplayTags.h"
#include "Game/D1GameModeDungeon.h"
#include "Inventory/D1InventoryComponent.h"
#include "Inventory/D1ItemData.h"
#include "GameFramework/PlayerState.h"

AD1Enemy::AD1Enemy()
{
	// 몬스터는 정교한 입력 반응성이 필요 없는 AI 기반 이동/전투라 기본값(100Hz)은 과함.
	// 부하테스트에서 GameNetDriver 리플리케이션 비용의 71%(D1Enemy 호출 73만+회)를 차지한 1순위 비용 → 20Hz로 낮춤.
	SetNetUpdateFrequency(20.f);

	// 거리 컬링: 탑다운 카메라(ArmLength 900, Pitch -55, FOV 90) 기준 화면에 보이는 지면 범위는
	// 캐릭터로부터 약 900~1100유닛 → 2000유닛이면 화면에 보이는 몬스터는 안전하게 포함하면서
	// 그보다 먼 다른 플레이어 그룹의 몬스터는 리플리케이션 대상에서 제외된다.
	// 기본값(15,000유닛)보다 훨씬 좁혀서, 분산 스폰된 플레이어 그룹 간 불필요한 복제를 차단.
	SetNetCullDistanceSquared(2000.f * 2000.f);

	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	AbilitySystemComponent = CreateDefaultSubobject<UD1AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
	GetMesh()->MarkRenderStateDirty();
	Weapon->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
	Weapon->MarkRenderStateDirty();

	AttributeSet = CreateDefaultSubobject<UD1AttributeSet>("AttributeSet");

	HealthBar = CreateDefaultSubobject<UWidgetComponent>("HealthBar");
	HealthBar->SetupAttachment(GetRootComponent());
}

void AD1Enemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority()) return;

	D1AIController = Cast<AD1AIController>(NewController);
	D1AIController->GetBlackboardComponent()->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
	D1AIController->RunBehaviorTree(BehaviorTree);
	D1AIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), false);

	bool bIsRanged = (CharacterClass == ECharacterClass::Goblin_Ranger);
	D1AIController->GetBlackboardComponent()->SetValueAsBool(FName("RangedAttacker"), bIsRanged);
}

int32 AD1Enemy::GetPlayerLevel_Implementation()
{
	return Level;
}

void AD1Enemy::Die()
{
	if (HasAuthority())
	{
		SetLifeSpan(LifeSpan);
		if (D1AIController)
		{
			D1AIController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
		}

		// 보스 사망 시 던전 클리어 처리
		if (bIsBoss)
		{
			if (AD1GameModeDungeon* GM = Cast<AD1GameModeDungeon>(GetWorld()->GetAuthGameMode()))
			{
				GM->OnBossDefeated();
			}
		}

		// 드롭 테이블 처리 — 모든 파티원에게 동일 지급 (co-op 공유 드롭)
		// 정상 흐름 로그는 Verbose로 — 죽음마다(Entry수×플레이어수) Warning을 동기 출력하던 게
		// 부하테스트에서 한 프레임에 수십 줄씩 몰려 CauseDamage 프레임을 수십ms 잡아먹는 원인이었음.
		UE_LOG(LogD1Inventory, Verbose, TEXT("Drop: Die() 서버 진입 — DropTable=%s"),
			DropTable ? *DropTable->GetName() : TEXT("NULL"));

		if (DropTable)
		{
			UE_LOG(LogD1Inventory, Verbose, TEXT("Drop: Entries 수=%d"), DropTable->Entries.Num());

			for (const FD1DropEntry& Entry : DropTable->Entries)
			{
				if (!IsValid(Entry.ItemData.Get()))
				{
					UE_LOG(LogD1Inventory, Warning, TEXT("Drop: ItemData NULL — 스킵"));
					continue;
				}
				if (FMath::FRandRange(0.f, 100.f) > Entry.DropChance)
				{
					UE_LOG(LogD1Inventory, Verbose, TEXT("Drop: %s 확률 미달 — 스킵"), *Entry.ItemData->ItemID.ToString());
					continue;
				}

				const int32 Quantity = FMath::RandRange(Entry.MinQuantity, Entry.MaxQuantity);

				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					if (APlayerController* PC = It->Get())
					{
						if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
						{
							if (UD1InventoryComponent* Inv = PS->FindComponentByClass<UD1InventoryComponent>())
							{
								const bool bOk = Inv->AddItem(Entry.ItemData->ItemID, Quantity);
								UE_LOG(LogD1Inventory, Verbose, TEXT("Drop: %s x%d → %s (성공=%d)"),
									*Entry.ItemData->ItemID.ToString(), Quantity, *PS->GetPlayerName(), bOk);
							}
							else
							{
								UE_LOG(LogD1Inventory, Warning, TEXT("Drop: %s — InventoryComponent 없음"), *PS->GetPlayerName());
							}
						}
					}
				}
			}
		}
	}
	Super::Die();
}

void AD1Enemy::SetCombatTarget_Implementation(AActor* InCombatTarget)
{
	CombatTarget = InCombatTarget;
}

AActor* AD1Enemy::GetCombatTarget_Implementation() const
{
	return CombatTarget;
}

ECharacterClass AD1Enemy::GetCharacterClass_Implementation()
{
	return CharacterClass;
}

void AD1Enemy::HighlightActor_Implementation()
{
	GetMesh()->SetRenderCustomDepth(true);
	Weapon->SetRenderCustomDepth(true);
}

void AD1Enemy::UnHighlightActor_Implementation()
{
	GetMesh()->SetRenderCustomDepth(false);
	Weapon->SetRenderCustomDepth(false);
}

void AD1Enemy::SetMoveToLocation_Implementation(FVector& OutDestination)
{
}

void AD1Enemy::HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bHitReacting = NewCount > 0;
	GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.f : BaseWalkSpeed;
	if (D1AIController && D1AIController->GetBlackboardComponent())
	{
		D1AIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), bHitReacting);
	}
}

void AD1Enemy::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	InitAbilityActorInfo();

	if (HasAuthority())
	{
		UD1AbilitySystemLibrary::GiveStartupAbilities(this, AbilitySystemComponent, CharacterClass);
	}

	if (UD1UserWidget* D1UserWidget = Cast<UD1UserWidget>(HealthBar->GetUserWidgetObject()))
	{
		D1UserWidget->SetWidgetController(this);
	}

	if (const UD1AttributeSet* D1AS = CastChecked<UD1AttributeSet>(AttributeSet))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(D1AS->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(Data.NewValue);
			}
		);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(D1AS->GetMaxHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChanged.Broadcast(Data.NewValue);
			}
		);

		AbilitySystemComponent->RegisterGameplayTagEvent(FD1GameplayTags::Get().Effects_HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(
			this,
			&AD1Enemy::HitReactTagChanged
		);

		// 시작할때 체력을 채워주는 초기화 코드
		OnHealthChanged.Broadcast(D1AS->GetHealth());
		OnMaxHealthChanged.Broadcast(D1AS->GetMaxHealth());
	}
}

void AD1Enemy::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<UD1AbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();

	if (HasAuthority())
	{
		InitializeDefaultAttributes();
	}
}

void AD1Enemy::InitializeDefaultAttributes() const
{
	UD1AbilitySystemLibrary::InitializeDefaultAttributes(this, CharacterClass, Level, AbilitySystemComponent);
}
