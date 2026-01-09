// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1Hero.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Player/D1PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"

AD1Hero::AD1Hero()
{
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
	InitAbilityActorInfo();

	InitializeDefaultAttributes();
}

void AD1Hero::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Init ability actor info for the Client
	InitAbilityActorInfo();
}

void AD1Hero::LoadProgress()
{
}

int32 AD1Hero::GetLevel() const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	return D1PS->GetPlayerLevel();
}

void AD1Hero::InitAbilityActorInfo()
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);

	D1PS->GetAbilitySystemComponent()->InitAbilityActorInfo(D1PS, this);

	AbilitySystemComponent = D1PS->GetAbilitySystemComponent();
	AttributeSet = D1PS->GetAttributeSet();
}
