
#include "UI/WidgetController/D1AttributeMenuWidgetController.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Data/D1AttributeInfo.h"
#include "D1GameplayTags.h"

void UD1AttributeMenuWidgetController::BroadcastInitialValues()
{
	UD1AttributeSet* AS = CastChecked<UD1AttributeSet>(AttributeSet);
	check(AttributeInfo);

	const FD1GameplayTags& Tags = FD1GameplayTags::Get();

	// --- Primary Attributes ---
	BroadcastAttributeInfo(Tags.Attributes_Primary_Strength, AS->GetStrengthAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Primary_Intelligence, AS->GetIntelligenceAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Primary_Dexterity, AS->GetDexterityAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Primary_Luck, AS->GetLuckAttribute());

	// --- Secondary Attributes ---
	BroadcastAttributeInfo(Tags.Attributes_Secondary_Armor, AS->GetArmorAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_ArmorPenetration, AS->GetArmorPenetrationAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_AttackPower, AS->GetAttackPowerAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_CriticalHitChance, AS->GetCriticalHitChanceAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_CriticalHitDamage, AS->GetCriticalHitDamageAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_HealthRegeneration, AS->GetHealthRegenerationAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_ManaRegeneration, AS->GetManaRegenerationAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_MaxHealth, AS->GetMaxHealthAttribute());
	BroadcastAttributeInfo(Tags.Attributes_Secondary_MaxMana, AS->GetMaxManaAttribute());
}

void UD1AttributeMenuWidgetController::BindCallbacksToDependencies()
{
	UD1AttributeSet* AS = CastChecked<UD1AttributeSet>(AttributeSet);
	const FD1GameplayTags& Tags = FD1GameplayTags::Get();

	// --- Primary 바인딩 ---
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetStrengthAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Primary_Strength, AS->GetStrengthAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetIntelligenceAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Primary_Intelligence, AS->GetIntelligenceAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetDexterityAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Primary_Dexterity, AS->GetDexterityAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetLuckAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Primary_Luck, AS->GetLuckAttribute()); });

	// --- Secondary 바인딩 ---
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetArmorAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_Armor, AS->GetArmorAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetArmorPenetrationAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_ArmorPenetration, AS->GetArmorPenetrationAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetAttackPowerAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_AttackPower, AS->GetAttackPowerAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetCriticalHitChanceAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_CriticalHitChance, AS->GetCriticalHitChanceAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetCriticalHitDamageAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_CriticalHitDamage, AS->GetCriticalHitDamageAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetHealthRegenerationAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_HealthRegeneration, AS->GetHealthRegenerationAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetManaRegenerationAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_ManaRegeneration, AS->GetManaRegenerationAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_MaxHealth, AS->GetMaxHealthAttribute()); });

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetMaxManaAttribute()).AddLambda(
		[this, Tags, AS](const FOnAttributeChangeData& Data) { BroadcastAttributeInfo(Tags.Attributes_Secondary_MaxMana, AS->GetMaxManaAttribute()); });
}

void UD1AttributeMenuWidgetController::BroadcastAttributeInfo(const FGameplayTag& AttributeTag, const FGameplayAttribute& Attribute) const
{
	FD1AttributeTagInfo Info = AttributeInfo->FindAttributeInfoForTag(AttributeTag);
	Info.AttributeValue = Attribute.GetNumericValue(AttributeSet);
	AttributeInfoDelegate.Broadcast(Info);
}