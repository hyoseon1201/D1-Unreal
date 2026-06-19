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
#include "AbilitySystem/Data/D1AbilityInfo.h"
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
#include "Game/D1HttpSubsystem.h"
#include "Player/D1PlayerState.h"
#include "Interaction/CombatInterface.h"
#include "Inventory/D1InventoryComponent.h"

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

void AD1PlayerController::SaveTravelDataToGameInstance()
{
	UD1GameInstance* GI = Cast<UD1GameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogD1Travel, Error, TEXT("SaveTravelData: GameInstance is NOT UD1GameInstance! Class=%s"),
			*GetNameSafe(GetGameInstance()));
		return;
	}

	AD1PlayerState* PS = GetPlayerState<AD1PlayerState>();
	if (!PS)
	{
		UE_LOG(LogD1Travel, Error, TEXT("SaveTravelData: PlayerState is NULL!"));
		return;
	}

	const UD1AttributeSet* AS = Cast<UD1AttributeSet>(PS->GetAttributeSet());
	if (!AS)
	{
		UE_LOG(LogD1Travel, Error, TEXT("SaveTravelData: AttributeSet is NULL!"));
		return;
	}

	TArray<FD1InventoryItem> InventorySlots;
	TArray<FD1EquippedItem> EquippedItems;
	if (UD1InventoryComponent* IC = PS->GetInventoryComponent())
	{
		InventorySlots = IC->GetInventorySlots();
		EquippedItems = IC->GetEquippedItems();
	}

	TArray<FD1SavedAbilityInfo> AbilityStates;
	if (UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(PS->GetAbilitySystemComponent()))
	{
		AbilityStates = D1ASC->SaveAbilityStates();
	}

	FD1SavedPlayerData Data;
	Data.AttributePoints = PS->GetAttributePoints();
	Data.SkillPoints     = PS->GetSkillPoints();
	Data.Level           = PS->GetPlayerLevel();
	Data.XP              = PS->GetXP();
	Data.Strength        = AS->GetStrength();
	Data.Intelligence    = AS->GetIntelligence();
	Data.Dexterity       = AS->GetDexterity();
	Data.Luck            = AS->GetLuck();
	Data.InventorySlots  = InventorySlots;
	Data.EquippedItems   = EquippedItems;
	Data.AbilityStates   = AbilityStates;

	const FString PlayerId = PS->GetPartyPlayerId();
	GI->SavePlayerData(PlayerId, Data);

	UE_LOG(LogD1Travel, Warning, TEXT("SaveTravelData [%s]: AttrPts=%d, SkillPts=%d, Level=%d, Str=%.1f, Abilities=%d"),
		*PlayerId, Data.AttributePoints, Data.SkillPoints, Data.Level, Data.Strength, AbilityStates.Num());
}

void AD1PlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	UE_LOG(LogD1Travel, Warning, TEXT("PreClientTravel: URL=%s, Type=%d — saving travel data as backup"),
		*PendingURL, (int32)TravelType);

	// 서버 측에서 이미 SaveTravelDataToGameInstance()를 호출했지만, 혹시 누락된 경우를 위한 보험
	// (이미 저장된 상태라면 덮어쓰기는 무해함)
	SaveTravelDataToGameInstance();
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

void AD1PlayerController::Server_ReturnToTown_Implementation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UD1HttpSubsystem* Http = GetGameInstance() ? GetGameInstance()->GetSubsystem<UD1HttpSubsystem>() : nullptr;

	// DB 로그인 여부로 크로스 프로세스 경로 결정 (요청자 기준)
	AD1PlayerState* RequesterPS = GetPlayerState<AD1PlayerState>();
	const bool bCrossProcess = Http != nullptr && RequesterPS && RequesterPS->WebCharacterId > 0;

	if (bCrossProcess)
	{
		// 크로스 프로세스: 던전 내 모든 플레이어를 Town으로 각자 ClientTravel
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			AD1PlayerController* PC = Cast<AD1PlayerController>(It->Get());
			if (!PC) continue;

			AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
			if (!PS || PS->WebCharacterId <= 0) continue;

			Http->SaveCharacter(PS->WebCharacterId, PS->BuildSaveJson());

			const int64 CharId = PS->WebCharacterId;
			TWeakObjectPtr<AD1PlayerController> WeakPC(PC);
			FD1IssueSessionDelegate Delegate;
			Delegate.BindLambda([WeakPC, CharId](bool bSuccess, const FString& Token, const FString& Address)
			{
				if (!bSuccess)
				{
					UE_LOG(LogD1Travel, Warning, TEXT("ReturnToTown: 세션 토큰 발급 실패 CharId=%lld"), CharId);
					return;
				}
				if (!WeakPC.IsValid()) return;

				const FString URL = FString::Printf(TEXT("%s?sessionToken=%s"), *Address, *Token);
				UE_LOG(LogD1Travel, Log, TEXT("ReturnToTown: ClientTravel → %s (CharId=%lld)"), *Address, CharId);
				WeakPC->ClientTravel(URL, ETravelType::TRAVEL_Absolute);
			});
			Http->IssueSessionToken(CharId, TEXT("town"), Delegate);
		}
	}
	else
	{
		// PIE 폴백: GameInstance TMap 저장 후 ServerTravel
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AD1PlayerController* PC = Cast<AD1PlayerController>(It->Get()))
			{
				PC->SaveTravelDataToGameInstance();
			}
		}
		World->ServerTravel(TEXT("/Game/Maps/Town"));
	}
}

void AD1PlayerController::ClientShowLoadingScreen_Implementation(const FText& LoadingText)
{
	// TODO: 로딩 위젯 표시 (Phase 2에서 WBP_Loading 구현)
	UE_LOG(LogD1UI, Log, TEXT("ShowLoadingScreen: %s"), *LoadingText.ToString());
}
