// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeBase.h"
#include "D1/D1.h"
#include "Game/D1GameStateBase.h"
#include "Player/D1PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Game/D1HttpSubsystem.h"

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

		PS->bIsTestBotConnection = UGameplayStatics::HasOption(Options, TEXT("testbot"));
	}

	return ErrorMessage;
}

void AD1GameModeBase::Logout(AController* Exiting)
{
	// 접속 종료 시 캐릭터 데이터 저장 (정상 종료 + 강제 킬 타임아웃 둘 다 여기로 들어옴).
	// HTTP는 비동기지만 본문을 동기 직렬화해 fire-and-forget하므로 PS 소멸과 무관하게 완료된다.
	if (AD1PlayerState* PS = Exiting ? Cast<AD1PlayerState>(Exiting->PlayerState) : nullptr)
	{
		if (PS->WebCharacterId > 0)
		{
			if (UD1HttpSubsystem* Http = GetGameInstance()->GetSubsystem<UD1HttpSubsystem>())
			{
				Http->SaveCharacter(PS->WebCharacterId, PS->BuildSaveJson());
				UE_LOG(LogD1Travel, Log, TEXT("Logout: CharId=%lld 저장 요청"), PS->WebCharacterId);
			}
		}
	}

	Super::Logout(Exiting);
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
