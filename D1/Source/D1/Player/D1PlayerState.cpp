// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/D1PlayerState.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"

AD1PlayerState::AD1PlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UD1AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UD1AttributeSet>("AttributeSet");

	SetNetUpdateFrequency(100.f);
}

void AD1PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

UAbilitySystemComponent* AD1PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
