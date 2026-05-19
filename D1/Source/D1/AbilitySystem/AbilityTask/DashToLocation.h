// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "DashToLocation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDashToLocationDelegate);

/**
 * 대시 이동을 수행하는 AbilityTask.
 * 서버에서만 실행되며, CMC의 Velocity를 이용해 부드럽게 이동한다.
 */
UCLASS()
class D1_API UDashToLocation : public UAbilityTask
{
	GENERATED_BODY()

public:
	/**
	 * 대시 AbilityTask를 생성한다.
	 * @param DashDistance 대시할 총 거리 (Unreal Unit)
	 * @param DashDuration 대시 지속 시간 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Task", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = true))
	static UDashToLocation* CreateDashToLocation(UGameplayAbility* OwningAbility, float DashDistance, float DashDuration);

	/** 대시가 목표 거리까지 완료되었을 때 발송 */
	UPROPERTY(BlueprintAssignable)
	FDashToLocationDelegate OnDashComplete;

	/** 대시 중 벽이나 장애물에 부딪혀 중단되었을 때 발송 */
	UPROPERTY(BlueprintAssignable)
	FDashToLocationDelegate OnHitWall;

	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityEnded) override;

private:
	/** 대시 시작 위치 (벽 체크용) */
	FVector StartLocation;

	/** 대시에 소요될 총 시간 */
	UPROPERTY()
	float Duration = 0.f;

	/** 대시 거리 */
	float DashDistance = 0.f;

	/** 대시 방향 */
	FVector DashDirection;

	/** 대시 속도 (Distance / Duration) */
	float DashSpeed = 0.f;

	/** 경과 시간 */
	float ElapsedTime = 0.f;

	/** 대시가 진행 중인지 여부 */
	bool bIsDashing = false;

	/** 대시를 수행 중인 캐릭터 */
	UPROPERTY()
	TWeakObjectPtr<ACharacter> CachedCharacter;

	/** 이전 프레임 위치 (벽 체크용) */
	FVector PreviousLocation;

	/** 연속으로 멈춘 틱 수 (벽 체크용) */
	int32 StuckTickCount = 0;

	/** 원래 GroundFriction 값 */
	float OriginalGroundFriction = 0.f;

	/** 원래 BrakingDecelerationWalking 값 */
	float OriginalBrakingDeceleration = 0.f;

	/** 원래 MovementMode */
	TEnumAsByte<EMovementMode> OriginalMovementMode = MOVE_Walking;

	/** Duration 후 종료를 위한 타이머 */
	FTimerHandle DashTimerHandle;

	UFUNCTION()
	void OnDashTimerExpired();

	void FinishDash(bool bHitWall);
};
