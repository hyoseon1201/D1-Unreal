// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeBase.h"
#include "D1/D1.h"
#include "Game/D1GameStateBase.h"
#include "Player/D1PlayerState.h"

AD1GameModeBase::AD1GameModeBase()
{
	GameStateClass = AD1GameStateBase::StaticClass();
}

void AD1GameModeBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AD1GameStateBase* D1GS = Cast<AD1GameStateBase>(GameState))
	{
		D1GS->ItemRegistry = ItemRegistry;
	}
}

void AD1GameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AD1PlayerState* PS = Cast<AD1PlayerState>(NewPlayer->PlayerState))
	{
		// GameMode의 Ability 허용 규칙을 PlayerState에 동기화 (클리언트에서도 접근 가능)
		PS->bAbilitiesAllowed = AreAbilitiesAllowed();

		UE_LOG(LogD1Travel, Verbose, TEXT("GameMode::PostLogin. PS=%p, bInit=%s, bAbilitiesAllowed=%s, AttrPoints=%d, Level=%d"),
			PS,
			PS->bAbilitySystemInitialized ? TEXT("TRUE") : TEXT("FALSE"),
			PS->bAbilitiesAllowed ? TEXT("TRUE") : TEXT("FALSE"),
			PS->GetAttributePoints(),
			PS->GetPlayerLevel());
	}
}
