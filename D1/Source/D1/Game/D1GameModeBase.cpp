// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeBase.h"
#include "Game/D1GameStateBase.h"

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
