// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1CharacterBase.h"

#include "D1/D1.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "D1GameplayTags.h"

AD1CharacterBase::AD1CharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);

	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UAbilitySystemComponent* AD1CharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FVector AD1CharacterBase::GetCombatSocketLocation_Implementation(const FGameplayTag& SocketTag)
{
	const FD1GameplayTags& GameplayTags = FD1GameplayTags::Get();
	// 1. 무기 끝 소켓 (칼 등)
	if (SocketTag.MatchesTagExact(GameplayTags.CombatSocket_Weapon) && IsValid(Weapon))
	{
		return Weapon->GetSocketLocation(WeaponTipSocketName);
	}

	// 2. 오른손 소켓
	if (SocketTag.MatchesTagExact(GameplayTags.CombatSocket_RightHand))
	{
		return GetMesh()->GetSocketLocation(RightHandSocketName);
	}

	// 3. 왼손 소켓 (필요시 추가)
	if (SocketTag.MatchesTagExact(GameplayTags.CombatSocket_LeftHand))
	{
		return GetMesh()->GetSocketLocation(LeftHandSocketName);
	}

	// 기본값으로 메쉬의 위치를 반환하거나 로그를 찍어 문제를 파악합니다.
	UE_LOG(LogTemp, Error, TEXT("Unknown Socket Tag: %s"), *SocketTag.ToString());
	return GetMesh()->GetComponentLocation();
}

void AD1CharacterBase::Die()
{
	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	MulticastHandleDeath();
}

bool AD1CharacterBase::IsDead_Implementation() const
{
	return bDead;
}

AActor* AD1CharacterBase::GetAvatar_Implementation()
{
	return this;
}

TArray<FTaggedMontage> AD1CharacterBase::GetAttackMontages_Implementation()
{
	return AttackMontages;
}

UAnimMontage* AD1CharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;
}

void AD1CharacterBase::MulticastHandleDeath_Implementation()
{
	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bDead = true;
}

void AD1CharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AD1CharacterBase::InitAbilityActorInfo()
{
}

void AD1CharacterBase::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	check(IsValid(GetAbilitySystemComponent()));
	check(GameplayEffectClass);
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

void AD1CharacterBase::InitializeDefaultAttributes() const
{
	int32 CurrentLevel = 1;

	if (this->Implements<UCombatInterface>())
	{
		CurrentLevel = ICombatInterface::Execute_GetPlayerLevel(const_cast<AD1CharacterBase*>(this));
	}

	ApplyEffectToSelf(DefaultPrimaryAttributes, (float)CurrentLevel);
	ApplyEffectToSelf(DefaultSecondaryAttributes, (float)CurrentLevel);
	ApplyEffectToSelf(DefaultVitalAttributes, (float)CurrentLevel);
}

void AD1CharacterBase::AddCharacterAbilities()
{
	UD1AbilitySystemComponent* D1ASC = CastChecked<UD1AbilitySystemComponent>(AbilitySystemComponent);
	if (!HasAuthority())
	{
		return;
	}

	D1ASC->AddCharacterAbilities(StartupAbilities);
}
