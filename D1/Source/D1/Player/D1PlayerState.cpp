// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/D1PlayerState.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

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

	DOREPLIFETIME(AD1PlayerState, Level);
	DOREPLIFETIME(AD1PlayerState, XP);
	DOREPLIFETIME(AD1PlayerState, AttributePoints);
	DOREPLIFETIME(AD1PlayerState, SkillPoints);
}

UAbilitySystemComponent* AD1PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AD1PlayerState::AddToXP(int32 InXP)
{
	XP += InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void AD1PlayerState::AddToLevel(int32 InLevel)
{
	Level += InLevel;
	OnLevelChangedDelegate.Broadcast(Level);
}

void AD1PlayerState::AddToAttributePoints(int32 InPoints)
{
	AttributePoints += InPoints;
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AD1PlayerState::AddToSkillPoints(int32 InPoints)
{
	SkillPoints += InPoints;
	OnSkillPointsChangedDelegate.Broadcast(SkillPoints);
}

void AD1PlayerState::SetXP(int32 InXP)
{
	XP = InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void AD1PlayerState::SetLevel(int32 InLevel)
{
	Level = InLevel;
	OnLevelChangedDelegate.Broadcast(Level);
}

void AD1PlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChangedDelegate.Broadcast(Level);
}

void AD1PlayerState::OnRep_XP(int32 OldXP)
{
	OnXPChangedDelegate.Broadcast(XP);
}

void AD1PlayerState::OnRep_AttributePoints(int32 OldAttributePoints)
{
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AD1PlayerState::OnRep_SkillPoints(int32 OldSkillPoints)
{
	OnSkillPointsChangedDelegate.Broadcast(SkillPoints);
}
