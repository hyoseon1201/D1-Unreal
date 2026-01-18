// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1CharacterBase.h"

#include "D1/D1.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"

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

FVector AD1CharacterBase::GetCombatSocketLocation_Implementation()
{
	check(Weapon);
	return Weapon->GetSocketLocation(WeaponTipSocketName);
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
