// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/AbilityTask/WhirlwindTick.h"

UWhirlwindTick* UWhirlwindTick::CreateWhirlwindTick(UGameplayAbility* OwningAbility, float TickInterval, float TotalDuration)
{
	UWhirlwindTick* MyObj = NewAbilityTask<UWhirlwindTick>(OwningAbility);
	MyObj->Interval = FMath::Max(TickInterval, KINDA_SMALL_NUMBER);
	MyObj->Duration = FMath::Max(TotalDuration, KINDA_SMALL_NUMBER);
	MyObj->ElapsedTime = 0.f;
	MyObj->TimeUntilNextTick = 0.f;
	MyObj->bIsRunning = false;
	return MyObj;
}

void UWhirlwindTick::Activate()
{
	Super::Activate();

	ElapsedTime = 0.f;
	TimeUntilNextTick = 0.f; // 시작하자마자 첫 틱 발송
	bIsRunning = true;

	// 매 프레임(0.016초)마다 체크하여 Interval 정확히 유지
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(TickTimerHandle, this, &UWhirlwindTick::TickTask, 0.016f, true);
	}
}

void UWhirlwindTick::TickTask()
{
	if (!bIsRunning)
	{
		return;
	}

	const float DeltaTime = 0.016f;
	ElapsedTime += DeltaTime;
	TimeUntilNextTick -= DeltaTime;

	// Interval마다 OnTick 발송 (시작 즉시 0초에도 1회 발송)
	if (TimeUntilNextTick <= 0.f)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnTick.Broadcast();
		}
		TimeUntilNextTick += Interval; // 누적 오차 방지
	}

	// Duration 종료
	if (ElapsedTime >= Duration)
	{
		FinishTask();
	}
}

void UWhirlwindTick::FinishTask()
{
	if (!bIsRunning)
	{
		return;
	}

	bIsRunning = false;

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnComplete.Broadcast();
	}

	EndTask();
}

void UWhirlwindTick::OnDestroy(bool AbilityEnded)
{
	bIsRunning = false;

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	Super::OnDestroy(AbilityEnded);
}
