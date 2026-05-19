// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/AbilityTask/DashToLocation.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UDashToLocation* UDashToLocation::CreateDashToLocation(UGameplayAbility* OwningAbility, float DashDistance, float DashDuration)
{
	UDashToLocation* MyObj = NewAbilityTask<UDashToLocation>(OwningAbility);

	if (OwningAbility)
	{
		const FGameplayAbilityActorInfo* ActorInfo = OwningAbility->GetCurrentActorInfo();
		if (ActorInfo && ActorInfo->AvatarActor.IsValid())
		{
			ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
			if (Character)
			{
				MyObj->CachedCharacter = Character;
				MyObj->DashDirection = Character->GetActorForwardVector();
				MyObj->DashDirection.Z = 0.f;
				MyObj->DashDirection.Normalize();
				MyObj->DashDistance = DashDistance;
				MyObj->Duration = FMath::Max(DashDuration, KINDA_SMALL_NUMBER);
				MyObj->DashSpeed = DashDistance / MyObj->Duration;
			}
		}
	}

	MyObj->bIsDashing = false;

	UE_LOG(LogTemp, Warning, TEXT("[DashToLocation] Created. Char=%s, Dist=%.1f, Dur=%.3f, Speed=%.1f, Dir=%s"),
		MyObj->CachedCharacter.IsValid() ? *MyObj->CachedCharacter->GetName() : TEXT("NULL"),
		MyObj->DashDistance, MyObj->Duration, MyObj->DashSpeed, *MyObj->DashDirection.ToCompactString());

	return MyObj;
}

void UDashToLocation::Activate()
{
	Super::Activate();

	if (!CachedCharacter.IsValid())
	{
		FinishDash(false);
		return;
	}

	// 서버와 클라이언트 모두에서 bIsDashing을 true로 설정해야
	// 클라이언트도 Duration 후 OnDashComplete를 받아서 Montage Jump 등을 처리할 수 있음
	bIsDashing = true;
	ElapsedTime = 0.f;

	// 서버에서만 실제 이동(CMC Velocity, 물리 상태 변경)을 처리함
	if (CachedCharacter->HasAuthority())
	{
		UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement();
		if (MoveComp)
		{
			OriginalGroundFriction = MoveComp->GroundFriction;
			OriginalBrakingDeceleration = MoveComp->BrakingDecelerationWalking;
			OriginalMovementMode = MoveComp->MovementMode;

			MoveComp->StopMovementImmediately();
			MoveComp->GroundFriction = 0.f;
			MoveComp->BrakingDecelerationWalking = 0.f;
			MoveComp->SetMovementMode(MOVE_Flying);

			FVector LaunchVelocity = DashDirection * DashSpeed;
			LaunchVelocity.Z = 0.f;
			MoveComp->Velocity = LaunchVelocity;
			MoveComp->UpdateComponentVelocity();

			StartLocation = CachedCharacter->GetActorLocation();

			UE_LOG(LogTemp, Warning, TEXT("[DashToLocation] Server Activate. Vel=%s, Start=%s"),
				*LaunchVelocity.ToCompactString(), *StartLocation.ToCompactString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DashToLocation] Client Activate. Waiting for server replicated position."));
	}

	// 서버/클라이언트 모두 Duration 후에 FinishDash 호출 (Montage Jump 동기화를 위해)
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(DashTimerHandle, this, &UDashToLocation::OnDashTimerExpired, Duration, false);
	}
}

void UDashToLocation::OnDashTimerExpired()
{
	if (!bIsDashing || !CachedCharacter.IsValid())
	{
		return;
	}

	// 서버에서만 실제 이동 거리를 체크하여 벽 충돌 판정
	bool bHitWall = false;
	if (CachedCharacter->HasAuthority())
	{
		const FVector CurrentLocation = CachedCharacter->GetActorLocation();
		const float ActualDist = FVector::Dist(StartLocation, CurrentLocation);
		bHitWall = (ActualDist < DashDistance * 0.7f);

		UE_LOG(LogTemp, Warning, TEXT("[DashToLocation] Server TimerExpired. ActualDist=%.1f, Expected=%.1f, bHitWall=%s"),
			ActualDist, DashDistance, bHitWall ? TEXT("TRUE") : TEXT("FALSE"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DashToLocation] Client TimerExpired. Skipping distance check."));
	}

	FinishDash(bHitWall);
}

void UDashToLocation::FinishDash(bool bHitWall)
{
	if (!bIsDashing)
	{
		return;
	}

	bIsDashing = false;

	// 타이머 정리
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(DashTimerHandle);
	}

	// 물리 상태 복구는 서버에서만 수행 (클라이언트는 서버의 Replicate를 받음)
	if (CachedCharacter.IsValid() && CachedCharacter->HasAuthority())
	{
		if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
		{
			MoveComp->Velocity = FVector::ZeroVector;
			MoveComp->GroundFriction = OriginalGroundFriction;
			MoveComp->BrakingDecelerationWalking = OriginalBrakingDeceleration;
			MoveComp->SetMovementMode(OriginalMovementMode);

			UE_LOG(LogTemp, Warning, TEXT("[DashToLocation] Server Cleanup. Mode restored to %d"), (int32)OriginalMovementMode);
		}
	}

	// 서버와 클라이언트 모두에서 Delegate 발송
	// (Montage Jump 등 로컬 처리를 위해 클라이언트도 받아야 함)
	if (bHitWall)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnHitWall.Broadcast();
		}
	}
	else
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnDashComplete.Broadcast();
		}
	}

	EndTask();
}

void UDashToLocation::OnDestroy(bool AbilityEnded)
{
	bIsDashing = false;

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(DashTimerHandle);
	}

	// OnDestroy는 클라이언트에서도 호출될 수 있으므로 Authority 체크
	if (CachedCharacter.IsValid() && CachedCharacter->HasAuthority())
	{
		if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
		{
			MoveComp->Velocity = FVector::ZeroVector;
			MoveComp->GroundFriction = OriginalGroundFriction;
			MoveComp->BrakingDecelerationWalking = OriginalBrakingDeceleration;
			MoveComp->SetMovementMode(OriginalMovementMode);
		}
	}

	Super::OnDestroy(AbilityEnded);
}
