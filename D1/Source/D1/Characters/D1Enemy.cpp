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

AD1Enemy::AD1Enemy()
{
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
}

int32 AD1Enemy::GetPlayerLevel_Implementation()
{
	return Level;
}

void AD1Enemy::Die()
{
	SetLifeSpan(LifeSpan);
	Super::Die();
}

void AD1Enemy::SetCombatTarget_Implementation(AActor* InCombatTarget)
{
}

AActor* AD1Enemy::GetCombatTarget_Implementation() const
{
	return nullptr;
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
}

void AD1Enemy::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	InitAbilityActorInfo();

	UD1AbilitySystemLibrary::GiveStartupAbilities(this, AbilitySystemComponent);

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
