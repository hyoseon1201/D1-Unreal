// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/D1Portal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"
#include "Game/D1HttpSubsystem.h"
#include "D1.h"

AD1Portal::AD1Portal()
{
	PrimaryActorTick.bCanEverTick = false;

	PortalCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalCollision"));
	PortalCollision->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	PortalCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PortalCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = PortalCollision;
}

void AD1Portal::BeginPlay()
{
	Super::BeginPlay();

	PortalCollision->OnComponentBeginOverlap.AddDynamic(this, &AD1Portal::OnPortalOverlap);
}

void AD1Portal::OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 이동 트리거(SaveCharacter, ClientTravel)는 권한을 가진 서버에서만 처리
	if (!HasAuthority())
	{
		return;
	}

	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn)
	{
		return;
	}

	AD1PlayerController* PC = Cast<AD1PlayerController>(OtherPawn->GetController());
	if (!PC)
	{
		return;
	}

	AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
	if (!PS)
	{
		return;
	}

	UE_LOG(LogD1Travel, Log, TEXT("D1Portal: %s 진입 (Destination=%s, FallbackMap=%s)"),
		*OtherActor->GetName(), *Destination, *DestinationMapName.ToString());

	UD1HttpSubsystem* Http = GetGameInstance() ? GetGameInstance()->GetSubsystem<UD1HttpSubsystem>() : nullptr;
	const bool bCrossProcess = Http != nullptr && PS->WebCharacterId > 0;

	if (bCrossProcess)
	{
		// 크로스 프로세스: Save → IssueToken → ClientTravel
		Http->SaveCharacter(PS->WebCharacterId, PS->BuildSaveJson());

		const int64 CharId = PS->WebCharacterId;
		const FName PlayerStartTag = DestinationPlayerStartTag;
		TWeakObjectPtr<AD1PlayerController> WeakPC(PC);
		FD1IssueSessionDelegate SessionDelegate;
		SessionDelegate.BindLambda([WeakPC, CharId, PlayerStartTag](bool bSuccess, const FString& Token, const FString& Address)
		{
			if (!bSuccess)
			{
				UE_LOG(LogD1Travel, Warning, TEXT("D1Portal: 세션 토큰 발급 실패 CharId=%lld"), CharId);
				return;
			}
			if (!WeakPC.IsValid())
			{
				UE_LOG(LogD1Travel, Warning, TEXT("D1Portal: PC가 이미 소멸됨 CharId=%lld"), CharId);
				return;
			}

			FString URL = FString::Printf(TEXT("%s?sessionToken=%s"), *Address, *Token);
			if (!PlayerStartTag.IsNone())
			{
				URL += FString::Printf(TEXT("#%s"), *PlayerStartTag.ToString());
			}
			UE_LOG(LogD1Travel, Log, TEXT("D1Portal: ClientTravel → %s (CharId=%lld)"), *Address, CharId);
			WeakPC->ClientTravel(URL, ETravelType::TRAVEL_Absolute);
		});
		Http->IssueSessionToken(CharId, Destination, SessionDelegate);
	}
	else
	{
		// PIE 폴백: GameInstance TMap 저장 후 ServerTravel (같은 프로세스 내 이동)
		PC->SaveTravelDataToGameInstance();

		FString TravelURL = DestinationMapName.ToString();
		if (!TravelURL.StartsWith(TEXT("/")))
		{
			TravelURL = FString::Printf(TEXT("/Game/Maps/%s"), *TravelURL);
		}
		UE_LOG(LogD1Travel, Log, TEXT("D1Portal: ServerTravel(PIE) → %s"), *TravelURL);
		GetWorld()->ServerTravel(TravelURL);
	}
}
