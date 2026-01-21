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

void UD1ProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation)
{
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && AvatarActor->Implements<UCombatInterface>())
	{
		const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(AvatarActor);

		FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
		Rotation.Pitch = 0.f;

		const float SpawnDistanceOffset = 150.f;
		const FVector ForwardDirection = Rotation.Vector();
		const FVector SpawnLocation = SocketLocation + (ForwardDirection * SpawnDistanceOffset);

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
		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), SourceASC->MakeEffectContext());

		FD1GameplayTags GameplayTags = FD1GameplayTags::Get();
		const float ScaledDamage = Damage.GetValueAtLevel(GetAbilityLevel());

		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Damage, ScaledDamage);
		Projectile->DamageEffectSpecHandle = SpecHandle;

		Projectile->FinishSpawning(SpawnTransform);
	}
}
