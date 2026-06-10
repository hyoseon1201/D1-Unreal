// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/D1PlayerController.h"

#include "D1/D1.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Input/D1InputComponent.h"
#include "Components/SplineComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "D1GameplayTags.h"
#include "Interaction/HighlightInterface.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Character.h"
#include "UI/Widget/D1DamageTextComponent.h"
#include "UI/Widget/D1UserWidget.h"
#include "UI/WidgetController/D1DungeonResultWidgetController.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "Game/D1GameInstance.h"
#include "Game/D1GameModeBase.h"
#include "Game/D1GameModeTown.h"
#include "Game/D1GameStateTown.h"
#include "Player/D1PlayerState.h"
#include "Interaction/CombatInterface.h"

AD1PlayerController::AD1PlayerController()
{
	bReplicates = true;
	Spline = CreateDefaultSubobject<USplineComponent>("Spline");
}

void AD1PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
	AutoRun();
}

void AD1PlayerController::ShowDamageNumber_Implementation(float DamageAmount, ACharacter* TargetCharacter, bool bCriticalHit)
{
	if (IsValid(TargetCharacter) && DamageTextComponentClass && IsLocalController())
	{
		UD1DamageTextComponent* DamageText = NewObject<UD1DamageTextComponent>(TargetCharacter, DamageTextComponentClass);
		DamageText->RegisterComponent();
		DamageText->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		DamageText->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		DamageText->SetDamageText(DamageAmount, bCriticalHit);
	}
}

void AD1PlayerController::BeginPlay()
{
	Super::BeginPlay();

	check(D1Context);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(D1Context, 0);
	}

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);
}

void AD1PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UD1InputComponent* D1InputComponent = CastChecked<UD1InputComponent>(InputComponent);
	D1InputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AD1PlayerController::Move);
	D1InputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
}

void AD1PlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}

void AD1PlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FD1GameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}

	if (InputTag.MatchesTagExact(FD1GameplayTags::Get().InputTag_RMB))
	{
		bTargeting = ThisActor ? true : false;
		bAutoRunning = false;
	}

	// ★ 마을(Town)에서는 전투/버프/회복 등 Ability 사용 불가
	// RMB(우클릭 이동)은 Ability가 아니므로 제외
	if (!InputTag.MatchesTagExact(FD1GameplayTags::Get().InputTag_RMB) && !CanUseAbilities())
	{
		return;
	}

	if (GetASC())
	{
		GetASC()->AbilityInputTagPressed(InputTag);
	}
}

void AD1PlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FD1GameplayTags::Get().Player_Block_InputReleased))
	{
		return;
	}

	if (!InputTag.MatchesTagExact(FD1GameplayTags::Get().InputTag_RMB))
	{
		// ★ 마을(Town)에서는 Ability 사용 불가
		if (CanUseAbilities() && GetASC())
		{
			GetASC()->AbilityInputTagReleased(InputTag);
		}
		return;
	}

	if (bTargeting)
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagReleased(InputTag);
		}
	}
	else
	{
		// GAS Ability 실행 중(예: 대시, 스턴 등)에는 우클릭으로 AutoRun을 시작하지 않도록 태그 체크
		if (GetASC() && GetASC()->HasMatchingGameplayTag(FD1GameplayTags::Get().Player_Block_InputPressed))
		{
			FollowTime = 0.f;
		}
		else
		{
			APawn* ControlledPawn = GetPawn();
			if (FollowTime <= ShortPressThreshold && ControlledPawn)
			{
				if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, ControlledPawn->GetActorLocation(), CachedDestination))
				{
					Spline->ClearSplinePoints();
					for (const FVector& PointLoc : NavPath->PathPoints)
					{
						Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
					}
					if (NavPath->PathPoints.Num() > 0)
					{
						CachedDestination = NavPath->PathPoints[NavPath->PathPoints.Num() - 1];
						bAutoRunning = true;
					}
				}
			}

			FollowTime = 0.f;
			bTargeting = false;
		}
	}
}

void AD1PlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FD1GameplayTags::Get().Player_Block_InputHeld))
	{
		return;
	}

	if (!InputTag.MatchesTagExact(FD1GameplayTags::Get().InputTag_RMB))
	{
		// ★ 마을(Town)에서는 Ability 사용 불가
		if (CanUseAbilities() && GetASC())
		{
			GetASC()->AbilityInputTagHeld(InputTag);
		}
		return;
	}

	if (bTargeting)
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagHeld(InputTag);
		}
	}
	else
	{
		FollowTime += GetWorld()->GetDeltaSeconds();

		if (CursorHit.bBlockingHit)
		{
			CachedDestination = CursorHit.ImpactPoint;
		}

		if (APawn* ControlledPawn = GetPawn())
		{
			const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
			ControlledPawn->AddMovementInput(WorldDirection);
		}
	}
}

UD1AbilitySystemComponent* AD1PlayerController::GetASC()
{
	if (D1AbilitySystemComponent == nullptr)
	{
		D1AbilitySystemComponent = Cast<UD1AbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return D1AbilitySystemComponent;
}

void AD1PlayerController::CursorTrace()
{
	GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	if (!CursorHit.bBlockingHit) return;

	LastActor = ThisActor;

	if (IsValid(CursorHit.GetActor()) && CursorHit.GetActor()->Implements<UHighlightInterface>())
	{
		ThisActor = CursorHit.GetActor();
	}
	else
	{
		ThisActor = nullptr;
	}

	if (LastActor != ThisActor)
	{
		UnHighlightActor(LastActor);
		HighlightActor(ThisActor);
	}
}

void AD1PlayerController::HighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_HighlightActor(InActor);
	}
}

void AD1PlayerController::UnHighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_UnHighlightActor(InActor);
	}
}

void AD1PlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	UE_LOG(LogD1Travel, Verbose, TEXT("PreClientTravel. URL=%s, Type=%d, Seamless=%s"),
		*PendingURL, (int32)TravelType, bIsSeamlessTravel ? TEXT("TRUE") : TEXT("FALSE"));

	// ClientTravel 직전 PlayerState + Primary Attribute 데이터를 GameInstance에 저장
	if (UD1GameInstance* GI = Cast<UD1GameInstance>(GetGameInstance()))
	{
		if (AD1PlayerState* PS = GetPlayerState<AD1PlayerState>())
		{
			if (const UD1AttributeSet* AS = Cast<UD1AttributeSet>(PS->GetAttributeSet()))
			{
				GI->SavePlayerStateData(
					PS->GetAttributePoints(), PS->GetPlayerLevel(), PS->GetXP(),
					AS->GetStrength(), AS->GetIntelligence(),
					AS->GetDexterity(), AS->GetLuck());
			}
			else
			{
				UE_LOG(LogD1Travel, Error, TEXT("PreClientTravel: AttributeSet is NULL!"));
			}
		}
		else
		{
			UE_LOG(LogD1Travel, Error, TEXT("PreClientTravel: PlayerState is NULL!"));
		}
	}
	else
	{
		UE_LOG(LogD1Travel, Error, TEXT("PreClientTravel: GameInstance is NOT UD1GameInstance! GetGameInstance()=%s"),
			*GetNameSafe(GetGameInstance()));
	}
}

void AD1PlayerController::TravelToMap(const FString& MapName)
{
	// PreClientTravel이 자동으로 호출되므로 별도 저장 불필요
	ClientTravel(MapName, TRAVEL_Absolute);
}

bool AD1PlayerController::CanUseAbilities() const
{
	if (const AD1PlayerState* PS = GetPlayerState<AD1PlayerState>())
	{
		return PS->bAbilitiesAllowed;
	}
	return true; // PlayerState가 없으면 기본 허용
}

void AD1PlayerController::ClientShowDungeonResult_Implementation(const TArray<FText>& AcquiredItems)
{
	if (!IsLocalController())
	{
		return;
	}

	if (!DungeonResultWidgetClass)
	{
		UE_LOG(LogD1UI, Error, TEXT("ClientShowDungeonResult: DungeonResultWidgetClass is NULL! Assign WBP_DungeonResult in BP_D1PlayerController"));
		return;
	}

	// WidgetController 패턴으로 DungeonResultWidget 생성
	UD1DungeonResultWidgetController* WC = UD1AbilitySystemLibrary::GetDungeonResultWidgetController(this);
	if (!WC)
	{
		UE_LOG(LogD1UI, Error, TEXT("ClientShowDungeonResult: GetDungeonResultWidgetController returned NULL!"));
		return;
	}

	UUserWidget* ResultWidget = CreateWidget<UUserWidget>(this, DungeonResultWidgetClass);
	if (!ResultWidget)
	{
		UE_LOG(LogD1UI, Error, TEXT("ClientShowDungeonResult: CreateWidget returned NULL! WidgetClass=%s"), *GetNameSafe(DungeonResultWidgetClass));
		return;
	}

	// 위젯에 WC 연결 (UD1UserWidget 기반이면 SetWidgetController 호출)
	if (UD1UserWidget* D1Widget = Cast<UD1UserWidget>(ResultWidget))
	{
		D1Widget->SetWidgetController(WC);
	}

	ResultWidget->AddToViewport();

	// 바인딩 완료 후 획득 아이템 데이터 세팅 → Delegate 발송
	WC->SetAcquiredItems(AcquiredItems);

	// 입력 모드를 UI 전용으로 변경
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	UE_LOG(LogD1UI, Verbose, TEXT("ClientShowDungeonResult: shown with %d items"), AcquiredItems.Num());
}

void AD1PlayerController::AutoRun()
{
	if (!bAutoRunning) return;

	// GAS Ability 실행 중(예: 대시, 스턴 등)에는 AutoRun을 즉시 중단
	if (GetASC() && (GetASC()->HasMatchingGameplayTag(FD1GameplayTags::Get().Player_Block_InputPressed) ||
					 GetASC()->HasMatchingGameplayTag(FD1GameplayTags::Get().Player_Block_InputHeld)))
	{
		bAutoRunning = false;
		return;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		const FVector LocationOnSpline = Spline->FindLocationClosestToWorldLocation(ControlledPawn->GetActorLocation(), ESplineCoordinateSpace::World);
		const FVector Direction = Spline->FindDirectionClosestToWorldLocation(LocationOnSpline, ESplineCoordinateSpace::World);
		ControlledPawn->AddMovementInput(Direction);

		const float DistanceToDestination = (LocationOnSpline - CachedDestination).Length();
		if (DistanceToDestination <= AutoRunAcceptanceRadius)
		{
			bAutoRunning = false;
		}
	}
}

void AD1PlayerController::ShowDungeonEntryUI()
{
	if (!IsLocalController())
	{
		return;
	}

	// 이미 생성된 위젯이 뷰포트에 있으면 중복 생성하지 않음
	if (DungeonEntryWidgetInstance && DungeonEntryWidgetInstance->IsInViewport())
	{
		return;
	}

	if (!DungeonEntryWidgetClass)
	{
		UE_LOG(LogD1UI, Error, TEXT("ShowDungeonEntryUI: DungeonEntryWidgetClass is NULL! Assign WBP_DungeonEntry in BP_D1PlayerController"));
		return;
	}

	DungeonEntryWidgetInstance = CreateWidget<UUserWidget>(this, DungeonEntryWidgetClass);
	if (!DungeonEntryWidgetInstance)
	{
		UE_LOG(LogD1UI, Error, TEXT("ShowDungeonEntryUI: CreateWidget failed!"));
		return;
	}

	DungeonEntryWidgetInstance->AddToViewport();

	// UI 입력 모드로 전환
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	UE_LOG(LogD1UI, Verbose, TEXT("ShowDungeonEntryUI completed."));
}

void AD1PlayerController::Server_CreateParty_Implementation(const FString& DungeonMap, const FString& PartyName)
{
	AD1PlayerState* PS = GetPlayerState<AD1PlayerState>();
	AD1GameStateTown* GSTown = Cast<AD1GameStateTown>(GetWorld()->GetGameState());
	if (!PS || !GSTown) return;

	// 캐릭터 레벨 읽기 (CombatInterface)
	int32 PlayerLevel = 1;
	if (APawn* MyPawn = GetPawn())
	{
		if (MyPawn->Implements<UCombatInterface>())
		{
			PlayerLevel = ICombatInterface::Execute_GetPlayerLevel(MyPawn);
		}
	}

	GSTown->ServerCreateParty(PS->GetPartyPlayerId(), PS->GetPlayerName(), PlayerLevel, DungeonMap, PartyName);
}

void AD1PlayerController::Server_JoinParty_Implementation(int32 PartyId)
{
	AD1PlayerState* PS = GetPlayerState<AD1PlayerState>();
	AD1GameStateTown* GSTown = Cast<AD1GameStateTown>(GetWorld()->GetGameState());
	if (!PS || !GSTown) return;

	// 캐릭터 레벨 읽기 (CombatInterface)
	int32 PlayerLevel = 1;
	if (APawn* MyPawn = GetPawn())
	{
		if (MyPawn->Implements<UCombatInterface>())
		{
			PlayerLevel = ICombatInterface::Execute_GetPlayerLevel(MyPawn);
		}
	}

	GSTown->ServerJoinParty(PartyId, PS->GetPartyPlayerId(), PS->GetPlayerName(), PlayerLevel);
}

void AD1PlayerController::Server_LeaveParty_Implementation()
{
	if (AD1PlayerState* PS = GetPlayerState<AD1PlayerState>())
	{
		if (AD1GameStateTown* GSTown = Cast<AD1GameStateTown>(GetWorld()->GetGameState()))
		{
			const FString PlayerId = PS->GetPartyPlayerId();
			if (FD1PartyInfo* Party = GSTown->FindPartyByPlayerId(PlayerId))
			{
				GSTown->ServerLeaveParty(Party->PartyId, PlayerId);
			}
		}
	}
}

void AD1PlayerController::Server_SetReady_Implementation(bool bReady)
{
	if (AD1PlayerState* PS = GetPlayerState<AD1PlayerState>())
	{
		if (AD1GameStateTown* GSTown = Cast<AD1GameStateTown>(GetWorld()->GetGameState()))
		{
			const FString PlayerId = PS->GetPartyPlayerId();
			if (FD1PartyInfo* Party = GSTown->FindPartyByPlayerId(PlayerId))
			{
				GSTown->ServerSetReady(Party->PartyId, PlayerId, bReady);
			}
		}
	}
}

void AD1PlayerController::Server_SetSelectedDungeon_Implementation(const FString& DungeonMap)
{
	if (AD1PlayerState* PS = GetPlayerState<AD1PlayerState>())
	{
		if (AD1GameStateTown* GSTown = Cast<AD1GameStateTown>(GetWorld()->GetGameState()))
		{
			const FString PlayerId = PS->GetPartyPlayerId();
			if (FD1PartyInfo* Party = GSTown->FindPartyByPlayerId(PlayerId))
			{
				if (Party->IsLeader(PlayerId))
				{
					GSTown->ServerSelectDungeon(Party->PartyId, DungeonMap);
				}
			}
		}
	}
}

void AD1PlayerController::Server_StartDungeon_Implementation()
{
	if (AD1GameModeTown* GameModeTown = Cast<AD1GameModeTown>(GetWorld()->GetAuthGameMode()))
	{
		GameModeTown->StartDungeonForParty(this);
	}
}

void AD1PlayerController::ClientShowLoadingScreen_Implementation(const FText& LoadingText)
{
	// TODO: 로딩 위젯 표시 (Phase 2에서 WBP_Loading 구현)
	UE_LOG(LogD1UI, Log, TEXT("ShowLoadingScreen: %s"), *LoadingText.ToString());
}
