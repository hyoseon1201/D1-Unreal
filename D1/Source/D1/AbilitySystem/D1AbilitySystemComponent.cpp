// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/D1AbilitySystemComponent.h"

#include "D1GameplayTags.h"

void UD1AbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UD1AbilitySystemComponent::ClientEffectApplied);
}

void UD1AbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);

	EffectAssetTags.Broadcast(TagContainer);
}
