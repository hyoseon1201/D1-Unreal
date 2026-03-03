// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/D1Hero.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Player/D1PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "UI/HUD/D1HUD.h"
#include "Player/D1PlayerController.h"
#include "AbilitySystem/Data/D1LevelupInfo.h"

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
	AddCharacterAbilities();
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
	
}

int32 AD1Hero::FindLevelForXP_Implementation(int32 InXP) const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	return D1PS->LevelUpInfo->FindLevelForXP(InXP);
}

int32 AD1Hero::GetAttributePointsReward_Implementation(int32 Level) const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	return D1PS->LevelUpInfo->LevelupInformation[Level].AttributePointAward;
}

int32 AD1Hero::GetSpellPointsReward_Implementation(int32 Level) const
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	return D1PS->LevelUpInfo->LevelupInformation[Level].SpellPointAward;
}

void AD1Hero::AddToPlayerLevel_Implementation(int32 InPlayerLevel)
{
	AD1PlayerState* D1PS = GetPlayerState<AD1PlayerState>();
	check(D1PS);
	D1PS->AddToLevel(InPlayerLevel);
}

void AD1Hero::AddToSpellPoints_Implementation(int32 InSpellPoints)
{
	// TODO
}

void AD1Hero::AddToAttributePoints_Implementation(int32 InAttributePoints)
{
	// TODO
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
