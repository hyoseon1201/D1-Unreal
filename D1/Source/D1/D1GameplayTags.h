
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FD1GameplayTags
{
public:
	static const FD1GameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();

	/* Vital Attributes */
	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_Mana;

	/* Primary Attributes */
	FGameplayTag Attributes_Primary_Strength;
	FGameplayTag Attributes_Primary_Intelligence;
	FGameplayTag Attributes_Primary_Dexterity;
	FGameplayTag Attributes_Primary_Luck;

	/* Secondary Attributes */
	FGameplayTag Attributes_Secondary_AttackPower;
	FGameplayTag Attributes_Secondary_Armor;
	FGameplayTag Attributes_Secondary_ArmorPenetration;
	FGameplayTag Attributes_Secondary_CriticalHitChance;
	FGameplayTag Attributes_Secondary_CriticalHitDamage;
	FGameplayTag Attributes_Secondary_MaxHealth;
	FGameplayTag Attributes_Secondary_MaxMana;
	FGameplayTag Attributes_Secondary_HealthRegeneration;
	FGameplayTag Attributes_Secondary_ManaRegeneration;

private:
	static FD1GameplayTags GameplayTags;
};