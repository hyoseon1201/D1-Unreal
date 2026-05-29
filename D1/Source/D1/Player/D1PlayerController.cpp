// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/D1PlayerController.h"

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
#include "Player/D1PlayerState.h"

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
		UE_LOG(LogTemp, Warning, TEXT("[AbilityBlock] InputTag=%s BLOCKED (bAbilitiesAllowed=false)"), *InputTag.ToString());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AbilityInput] Pressed Tag=%s, CanUse=%s, HasASC=%s"),
		*InputTag.ToString(),
		CanUseAbilities() ? TEXT("TRUE") : TEXT("FALSE"),
		GetASC() ? TEXT("TRUE") : TEXT("FALSE"));

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
			UE_LOG(LogTemp, Warning, TEXT("[AbilityInput] Released Tag=%s, CanUse=TRUE"), *InputTag.ToString());
			GetASC()->AbilityInputTagReleased(InputTag);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AbilityInput] Released Tag=%s BLOCKED (CanUse=%s, HasASC=%s)"),
				*InputTag.ToString(),
				CanUseAbilities() ? TEXT("TRUE") : TEXT("FALSE"),
				GetASC() ? TEXT("TRUE") : TEXT("FALSE"));
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

	UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] PreClientTravel called. URL=%s, Type=%d, Seamless=%s"),
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
				UE_LOG(LogTemp, Error, TEXT("[TravelDebug] PreClientTravel: AttributeSet is NULL!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[TravelDebug] PreClientTravel: PlayerState is NULL!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TravelDebug] PreClientTravel: GameInstance is NOT UD1GameInstance! GetGameInstance()=%s"),
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
		UE_LOG(LogTemp, Warning, TEXT("[CanUseAbilities] PS found. bAbilitiesAllowed=%s"),
			PS->bAbilitiesAllowed ? TEXT("TRUE") : TEXT("FALSE"));
		return PS->bAbilitiesAllowed;
	}
	UE_LOG(LogTemp, Warning, TEXT("[CanUseAbilities] PS NOT found. Defaulting to TRUE"));
	return true; // PlayerState가 없으면 기본 허용
}

void AD1PlayerController::ClientShowDungeonResult_Implementation(const TArray<FText>& AcquiredItems)
{
	if (IsLocalController() && DungeonResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dungeon] Showing result widget on client. Items=%d"), AcquiredItems.Num());

		// WidgetController 패턴으로 DungeonResultWidget 생성
		if (UD1DungeonResultWidgetController* WC = UD1AbilitySystemLibrary::GetDungeonResultWidgetController(this))
		{
			UUserWidget* ResultWidget = CreateWidget<UUserWidget>(this, DungeonResultWidgetClass);
			if (ResultWidget)
			{
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
			}
		}
	}
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
