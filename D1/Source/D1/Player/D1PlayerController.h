// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "D1PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UD1InputConfig;
class USplineComponent;
class UD1AbilitySystemComponent;
class UD1DamageTextComponent;
struct FInputActionValue;
class AD1GameModeBase;

/**
 *
 */
UCLASS()
class D1_API AD1PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AD1PlayerController();
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(Client, Reliable)
	void ShowDamageNumber(float DamageAmount, ACharacter* TargetCharacter, bool bCriticalHit);

	/** 지정된 맵으로 ClientTravel을 실행한다. (BP_DungeonPortal 등에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "D1|Travel")
	void TravelToMap(const FString& MapName);

	/** PlayerState/Inventory 데이터를 GameInstance에 저장 (ServerTravel 직전에 서버 측에서 호출) */
	void SaveTravelDataToGameInstance();

	/** 현재 GameMode에서 Ability 사용이 허용되는가? (Town=false, Dungeon=true) */
	UFUNCTION(BlueprintPure, Category = "D1|GameMode Rules")
	bool CanUseAbilities() const;

	/** 던전 보스 처치 시 클라이언트에게 결과 위젯을 띄우라고 지시 (획득 아이템 데이터 포함) */
	UFUNCTION(Client, Reliable, Category = "D1|Dungeon")
	void ClientShowDungeonResult(const TArray<FText>& AcquiredItems);

	// ─── 파티 Server RPC (WidgetController / Blueprint에서 호출) ───────────

	/** 파티 생성 요청 — 본인이 파티장이 됨. 생성 시점에 선택된 던전과 방 제목을 함께 지정 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "D1|Party")
	void Server_CreateParty(const FString& DungeonMap, const FString& PartyName);

	/** 특정 파티에 참가 요청 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "D1|Party")
	void Server_JoinParty(int32 PartyId);

	/** 현재 파티에서 탈퇴 (파티장이면 파티 해산) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "D1|Party")
	void Server_LeaveParty();

	/** 준비 상태 설정 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "D1|Party")
	void Server_SetReady(bool bReady);

	/** 던전 선택 (파티장 전용) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "D1|Party")
	void Server_SetSelectedDungeon(const FString& DungeonMap);

	/** 던전 시작 (파티장 전용, 모든 파티원 이동) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "D1|Party")
	void Server_StartDungeon();

	/** 던전 클리어 후 마을 복귀 (WBP_DungeonResult의 Return to Town 버튼에서 호출) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "D1|Party")
	void Server_ReturnToTown();

protected:
	virtual void BeginPlay() override;

	/** ClientTravel 직전에 호출됨. PlayerState 데이터를 GameInstance에 저장 */
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> D1Context;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	void Move(const FInputActionValue& InputActionValue);

	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UD1InputConfig> InputConfig;

	UPROPERTY()
	TObjectPtr<UD1AbilitySystemComponent> D1AbilitySystemComponent;

	UD1AbilitySystemComponent* GetASC();

	void CursorTrace();
	TObjectPtr<AActor> LastActor;
	TObjectPtr<AActor> ThisActor;
	FHitResult CursorHit;

	static void HighlightActor(AActor* InActor);
	static void UnHighlightActor(AActor* InActor);

	void AutoRun();

	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.f;
	float ShortPressThreshold = 0.5f;
	bool bAutoRunning = false;
	bool bTargeting = false;

	UPROPERTY(EditDefaultsOnly)
	float AutoRunAcceptanceRadius = 50.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UD1DamageTextComponent> DamageTextComponentClass;

	/** 던전 클리어 시 표시할 결과 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> DungeonResultWidgetClass;

	/** 던전 입장 UI 위젯 클래스 (WBP_DungeonEntry) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> DungeonEntryWidgetClass;

	/** 던전 입장 UI 위젯 캐시 (중복 생성 방지) */
	UPROPERTY()
	TObjectPtr<UUserWidget> DungeonEntryWidgetInstance;

	/** 던전 입장 UI를 화면에 표시 (BP_DungeonPortal BeginOverlap에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "D1|UI")
	void ShowDungeonEntryUI();

	/** 클라이언트에게 로딩 화면 표시 지시 */
	UFUNCTION(Client, Reliable, Category = "D1|UI")
	void ClientShowLoadingScreen(const FText& LoadingText);

	// ─── 부하 테스트용 자동전투 봇 ───────────
	// 커맨드라인 -testbot으로 접속 즉시 자동 시작하거나, 콘솔에서 ToggleTestBot으로 원하는 시점에 수동 토글 가능.

	/** 콘솔 명령어로 자동전투 봇 On/Off (예: 로그인 후 원하는 위치로 직접 이동시킨 다음 켜기) */
	UFUNCTION(Exec, Category = "Test Bot")
	void ToggleTestBot();

	/** 데미지 계산은 서버 권한이라, 서버 쪽 ASC에 무적 태그를 직접 추가/제거해야 함 */
	UFUNCTION(Server, Reliable, Category = "Test Bot")
	void Server_SetTestBotInvulnerable(bool bEnable);

	/** 주기적으로 가장 가까운 적을 찾아 추격/공격한다 */
	void TickTestBot();

public:
	/**
	 * 테스트 봇이 현재 타게팅 중인 적을 가리키는 HitResult를 만든다.
	 * 헤드리스(-nullrhi) 봇은 마우스 커서가 없어 GetHitResultUnderCursor가 쓰레기값을 반환하므로,
	 * TargetDataUnderMouse가 커서 대신 이 값을 쓰도록 한다. 봇이 아니거나 타겟이 없으면 false.
	 * (외부 어빌리티 태스크에서 호출하므로 public)
	 */
	bool GetTestBotTargetHit(FHitResult& OutHit) const;

private:
	bool bIsTestBot = false;

	/** TickTestBot이 매 틱 갱신하는 현재 추격/공격 대상 (TargetDataUnderMouse에서 참조) */
	TWeakObjectPtr<AActor> TestBotTarget;

	FTimerHandle TestBotTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Test Bot")
	float TestBotTickInterval = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Test Bot")
	float TestBotDetectionRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Test Bot")
	float TestBotAttackRange = 150.f;
};
