// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WhirlwindTick.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWhirlwindTickDelegate);

/**
 * 일정 시간 동안 주기적으로 Tick delegate를 발송하는 AbilityTask.
 * 가렌 E(Judgment) 스타일의 지속 범위 데미지에 사용.
 */
UCLASS()
class D1_API UWhirlwindTick : public UAbilityTask
{
	GENERATED_BODY()

public:
	/**
	 * WhirlwindTick AbilityTask를 생성한다.
	 * @param TickInterval 데미지/틱 간격 (초)
	 * @param TotalDuration 총 지속 시간 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Task", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = true))
	static UWhirlwindTick* CreateWhirlwindTick(UGameplayAbility* OwningAbility, float TickInterval, float TotalDuration);

	/** 매 틱(Interval)마다 발송 — BP에서 CauseDamage 등을 연결 */
	UPROPERTY(BlueprintAssignable)
	FWhirlwindTickDelegate OnTick;

	/** 전체 Duration이 끝났을 때 발송 */
	UPROPERTY(BlueprintAssignable)
	FWhirlwindTickDelegate OnComplete;

	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityEnded) override;

private:
	/** 틱 간격 */
	float Interval = 0.5f;

	/** 총 지속 시간 */
	float Duration = 3.0f;

	/** 경과 시간 */
	float ElapsedTime = 0.f;

	/** 다음 틱까지 남은 시간 */
	float TimeUntilNextTick = 0.f;

	/** 실행 중인지 여부 */
	bool bIsRunning = false;

	/** 타이머 */
	FTimerHandle TickTimerHandle;

	UFUNCTION()
	void TickTask();  // Timer callback (not overriding UGameplayTask::TickTask)

	void FinishTask();
};
