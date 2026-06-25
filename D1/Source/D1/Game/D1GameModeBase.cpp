// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameModeBase.h"
#include "D1/D1.h"
#include "Game/D1GameStateBase.h"
#include "Player/D1PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Game/D1HttpSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Characters/D1Enemy.h"

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

void AD1GameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만, 주기적으로 살아있는 Enemy 수를 로그로 남긴다 (부하 테스트 관찰용).
	// Enemy가 없는 맵(Town 등)에서는 LogMonsterCount가 알아서 로그를 생략한다.
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(MonsterCountTimerHandle, this, &AD1GameModeBase::LogMonsterCount, MonsterCountLogInterval, true);
	}
}

void AD1GameModeBase::LogMonsterCount()
{
	int32 Count = 0;
	for (TActorIterator<AD1Enemy> It(GetWorld()); It; ++It)
	{
		++Count;
	}

	// 몬스터가 없는 맵(Town 등)은 매번 0을 찍어 로그를 더럽히므로 건너뛴다.
	if (Count == 0)
	{
		return;
	}

	MonsterCountSum += Count;
	++MonsterCountSamples;
	const double Average = MonsterCountSamples > 0 ? static_cast<double>(MonsterCountSum) / MonsterCountSamples : 0.0;

	UE_LOG(LogD1, Log, TEXT("[몬스터수] 현재=%d, 평균=%.1f (샘플 %d개, %.0f초 주기)"),
		Count, Average, MonsterCountSamples, MonsterCountLogInterval);
}

AActor* AD1GameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	const AD1PlayerState* PS = Player ? Player->GetPlayerState<AD1PlayerState>() : nullptr;
	if (PS && PS->bIsTestBotConnection)
	{
		// 부하 테스트 봇은 "Bot1"~"BotN" 태그가 달린 PlayerStart에 순서대로 스폰한다.
		// (몬스터 밀집 구역에 미리 배치해두면 봇이 항상 전투 한가운데서 시작 — 표류/idle 방지)
		const int32 BotIndex = ++TestBotSpawnIndex; // 1-based
		const FName WantedTag(*FString::Printf(TEXT("Bot%d"), BotIndex));

		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(WantedTag))
			{
				UE_LOG(LogD1Travel, Log, TEXT("ChoosePlayerStart: 테스트 봇 %s 태그 PlayerStart에 스폰"), *WantedTag.ToString());
				return *It;
			}
		}

		UE_LOG(LogD1Travel, Warning, TEXT("ChoosePlayerStart: '%s' 태그 PlayerStart를 못 찾음 (봇 수 > 태그 PlayerStart 수?), 기본 로직으로 폴백"), *WantedTag.ToString());
	}

	return Super::ChoosePlayerStart_Implementation(Player);
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
