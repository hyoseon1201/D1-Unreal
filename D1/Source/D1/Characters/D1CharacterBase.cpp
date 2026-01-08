// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1CharacterBase.h"

#include "AbilitySystemComponent.h"

AD1CharacterBase::AD1CharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UAbilitySystemComponent* AD1CharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
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
	ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	//ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);
	//ApplyEffectToSelf(DefaultVitalAttributes, 1.f);
}
