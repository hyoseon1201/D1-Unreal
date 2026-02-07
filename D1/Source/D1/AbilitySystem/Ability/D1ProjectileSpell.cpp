// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/D1ProjectileSpell.h"

#include "Actor/D1Projectile.h"
#include "Interaction/CombatInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "D1GameplayTags.h"

void UD1ProjectileSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UD1ProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag)
{
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && AvatarActor->Implements<UCombatInterface>())
	{
		FVector AdjustedTarget = ProjectileTargetLocation;
		AdjustedTarget.Z += 50.f;

		const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(AvatarActor, SocketTag);

		FRotator Rotation = (AdjustedTarget - (SocketLocation + FVector(0.f, 0.f, 30.f))).Rotation();

		const float SpawnDistanceOffset = 150.f;
		const FVector ForwardDirection = Rotation.Vector();
		const FVector SpawnLocation = SocketLocation + (ForwardDirection * SpawnDistanceOffset) + FVector(0.f, 0.f, 70.f);

		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SpawnLocation);
		SpawnTransform.SetRotation(Rotation.Quaternion());

		AD1Projectile* Projectile = GetWorld()->SpawnActorDeferred<AD1Projectile>(
			ProjectileClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			Cast<APawn>(GetOwningActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		const UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());
		FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
		EffectContextHandle.SetAbility(this);
		EffectContextHandle.AddSourceObject(Projectile);
		TArray<TWeakObjectPtr<AActor>> Actors;
		Actors.Add(Projectile);
		EffectContextHandle.AddActors(Actors);
		FHitResult HitResult;
		EffectContextHandle.AddHitResult(HitResult);

		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContextHandle);

		FD1GameplayTags GameplayTags = FD1GameplayTags::Get();

		for (auto& Pair : DamageTypes)
		{
			const float ScaledDamage = Pair.Value.GetValueAtLevel(GetAbilityLevel());
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Pair.Key, ScaledDamage);
		}

		Projectile->DamageEffectSpecHandle = SpecHandle;
		Projectile->FinishSpawning(SpawnTransform);
	}
}
