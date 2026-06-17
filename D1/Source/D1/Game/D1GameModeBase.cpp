// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeBase.h"
#include "D1/D1.h"
#include "Game/D1GameStateBase.h"
#include "Player/D1PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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

FString AD1GameModeBase::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
	const FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// 접속 URL 옵션에서 세션 토큰 추출 → PlayerState에 보관 (verify-session에서 사용)
	if (AD1PlayerState* PS = NewPlayerController ? Cast<AD1PlayerState>(NewPlayerController->PlayerState) : nullptr)
	{
		PS->PendingSessionToken = UGameplayStatics::ParseOption(Options, TEXT("sessionToken"));
		UE_LOG(LogD1Travel, Log, TEXT("InitNewPlayer: sessionToken %s"),
			PS->PendingSessionToken.IsEmpty() ? TEXT("없음") : TEXT("캡처됨"));
	}

	return ErrorMessage;
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
