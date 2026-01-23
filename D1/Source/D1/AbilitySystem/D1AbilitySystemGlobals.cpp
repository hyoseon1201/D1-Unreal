// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/D1AbilitySystemGlobals.h"

#include "D1AbilityTypes.h"

FGameplayEffectContext* UD1AbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FD1GameplayEffectContext();
}