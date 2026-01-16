// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1Enemy.h"

#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/D1AttributeSet.h"

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

	AttributeSet = CreateDefaultSubobject<UD1AttributeSet>("AttributeSet");
}

void AD1Enemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority()) return;

	// TODO: AI Controller ÃÊ±âÈ­
}

int32 AD1Enemy::GetPlayerLevel_Implementation()
{
	return Level;
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

void AD1Enemy::BeginPlay()
{
	Super::BeginPlay();
	InitAbilityActorInfo();
}

void AD1Enemy::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		InitializeDefaultAttributes();
	}
}
