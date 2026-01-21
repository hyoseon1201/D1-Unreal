
#include "AbilitySystem/D1AttributeSet.h"

#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GameplayEffectAggregator.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "D1GameplayTags.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/D1PlayerController.h"

UD1AttributeSet::UD1AttributeSet() 
{
	const FD1GameplayTags& Tags = FD1GameplayTags::Get();

	/*
	 * Vital Attributes
	 */
	TagsToAttributes.Add(Tags.Attributes_Vital_Health, GetHealthAttribute);
	TagsToAttributes.Add(Tags.Attributes_Vital_Mana, GetManaAttribute);

	/*
	 * Primary Attributes
	 */
	TagsToAttributes.Add(Tags.Attributes_Primary_Strength, GetStrengthAttribute);
	TagsToAttributes.Add(Tags.Attributes_Primary_Intelligence, GetIntelligenceAttribute);
	TagsToAttributes.Add(Tags.Attributes_Primary_Dexterity, GetDexterityAttribute);
	TagsToAttributes.Add(Tags.Attributes_Primary_Luck, GetLuckAttribute);

	/*
	 * Secondary Attributes
	 */
	TagsToAttributes.Add(Tags.Attributes_Secondary_AttackPower, GetAttackPowerAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_Armor, GetArmorAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_ArmorPenetration, GetArmorPenetrationAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_CriticalHitChance, GetCriticalHitChanceAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_CriticalHitDamage, GetCriticalHitDamageAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_MaxHealth, GetMaxHealthAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_MaxMana, GetMaxManaAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_HealthRegeneration, GetHealthRegenerationAttribute);
	TagsToAttributes.Add(Tags.Attributes_Secondary_ManaRegeneration, GetManaRegenerationAttribute);
}

void UD1AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Vital
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, Mana, COND_None, REPNOTIFY_Always);

	// Primary
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, Dexterity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, Luck, COND_None, REPNOTIFY_Always);

	// Secondary
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, CriticalHitDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, HealthRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UD1AttributeSet, ManaRegeneration, COND_None, REPNOTIFY_Always);
}

void UD1AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	if (Attribute == GetManaAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
}

void UD1AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
		UE_LOG(LogTemp, Warning, TEXT("Changed Health on %s, Health: %f"), *Props.TargetAvatarActor->GetName(), GetHealth());
	}
	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalIncomingDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (LocalIncomingDamage > 0.f)
		{
			const float NewHealth = GetHealth() - LocalIncomingDamage;
			SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));

			const bool bFatal = NewHealth <= 0.f;

			if (bFatal)
			{
				ICombatInterface* CombatInterface = Cast<ICombatInterface>(Props.TargetAvatarActor);
				if (CombatInterface)
				{
					CombatInterface->Die();
				}
			}
			
			ShowFloatingText(Props, LocalIncomingDamage);
		}
	}
}

/* OnRep Implementations */
void UD1AttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, Health, OldHealth); }
void UD1AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, Mana, OldMana); }
void UD1AttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, Strength, OldStrength); }
void UD1AttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, Intelligence, OldIntelligence); }
void UD1AttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldDexterity) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, Dexterity, OldDexterity); }
void UD1AttributeSet::OnRep_Luck(const FGameplayAttributeData& OldLuck) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, Luck, OldLuck); }
void UD1AttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, AttackPower, OldAttackPower); }
void UD1AttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, Armor, OldArmor); }
void UD1AttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, ArmorPenetration, OldArmorPenetration); }
void UD1AttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, CriticalHitChance, OldCriticalHitChance); }
void UD1AttributeSet::OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, CriticalHitDamage, OldCriticalHitDamage); }
void UD1AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, MaxHealth, OldMaxHealth); }
void UD1AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, MaxMana, OldMaxMana); }
void UD1AttributeSet::OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, HealthRegeneration, OldHealthRegeneration); }
void UD1AttributeSet::OnRep_ManaRegeneration(const FGameplayAttributeData& OldManaRegeneration) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UD1AttributeSet, ManaRegeneration, OldManaRegeneration); }

void UD1AttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const
{
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if (IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
		if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceController = Pawn->GetController();
			}
		}
		if (Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}

	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
	}
}

void UD1AttributeSet::ShowFloatingText(const FEffectProperties& Props, float Damage) const
{
	if (Props.SourceCharacter != Props.TargetCharacter)
	{
		if (AD1PlayerController* PC = Cast<AD1PlayerController>(UGameplayStatics::GetPlayerController(Props.SourceCharacter, 0)))
		{
			PC->ShowDamageNumber(Damage, Props.TargetCharacter);
		}
	}
}
