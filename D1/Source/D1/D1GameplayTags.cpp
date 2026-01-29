
#include "D1GameplayTags.h"

#include "GameplayTagsManager.h"

FD1GameplayTags FD1GameplayTags::GameplayTags;

void FD1GameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	/* Vital Attributes */
	GameplayTags.Attributes_Vital_Health = Manager.AddNativeGameplayTag(FName("Attributes.Vital.Health"), FString("Amount of current Health"));
	GameplayTags.Attributes_Vital_Mana = Manager.AddNativeGameplayTag(FName("Attributes.Vital.Mana"), FString("Amount of current Mana"));

	/* Primary Attributes */
	GameplayTags.Attributes_Primary_Strength = Manager.AddNativeGameplayTag(FName("Attributes.Primary.Strength"), FString("Increases physical damage"));
	GameplayTags.Attributes_Primary_Intelligence = Manager.AddNativeGameplayTag(FName("Attributes.Primary.Intelligence"), FString("Increases magical damage"));
	GameplayTags.Attributes_Primary_Dexterity = Manager.AddNativeGameplayTag(FName("Attributes.Primary.Dexterity"), FString("Increases armor penetration and crit damage"));
	GameplayTags.Attributes_Primary_Luck = Manager.AddNativeGameplayTag(FName("Attributes.Primary.Luck"), FString("Increases critical hit chance and armor penetration"));

	/* Secondary Attributes */
	GameplayTags.Attributes_Secondary_AttackPower = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.AttackPower"), FString("Calculated Attack Power"));
	GameplayTags.Attributes_Secondary_Armor = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.Armor"), FString("Reduces damage taken, improves Block Chance"));
	GameplayTags.Attributes_Secondary_ArmorPenetration = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.ArmorPenetration"), FString("Ignores a percentage of target Armor"));
	GameplayTags.Attributes_Secondary_CriticalHitChance = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.CriticalHitChance"), FString("Chance to double damage plus critical bonus"));
	GameplayTags.Attributes_Secondary_CriticalHitDamage = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.CriticalHitDamage"), FString("Bonus damage added when a critical hit occurs"));
	GameplayTags.Attributes_Secondary_MaxHealth = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.MaxHealth"), FString("Maximum amount of Health obtainable"));
	GameplayTags.Attributes_Secondary_MaxMana = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.MaxMana"), FString("Maximum amount of Mana obtainable"));
	GameplayTags.Attributes_Secondary_HealthRegeneration = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.HealthRegeneration"), FString("Amount of Health regenerated every 1 second"));
	GameplayTags.Attributes_Secondary_ManaRegeneration = Manager.AddNativeGameplayTag(FName("Attributes.Secondary.ManaRegeneration"), FString("Amount of Mana regenerated every 1 second"));

	/* Input Tags */
	GameplayTags.InputTag_LMB = Manager.AddNativeGameplayTag(FName("InputTag.LMB"), FString("Input Tag for Left Mouse Button"));
	GameplayTags.InputTag_RMB = Manager.AddNativeGameplayTag(FName("InputTag.RMB"), FString("Input Tag for Right Mouse Button"));
	GameplayTags.InputTag_1 = Manager.AddNativeGameplayTag(FName("InputTag.1"), FString("Input Tag for 1 Key"));
	GameplayTags.InputTag_2 = Manager.AddNativeGameplayTag(FName("InputTag.2"), FString("Input Tag for 2 Key"));
	GameplayTags.InputTag_3 = Manager.AddNativeGameplayTag(FName("InputTag.3"), FString("Input Tag for 3 Key"));
	GameplayTags.InputTag_4 = Manager.AddNativeGameplayTag(FName("InputTag.4"), FString("Input Tag for 4 Key"));

	/* Combat Sockets */
	GameplayTags.CombatSocket_Weapon = Manager.AddNativeGameplayTag(FName("CombatSocket.Weapon"), FString("Combat Socket Weapon"));
	GameplayTags.CombatSocket_RightHand = Manager.AddNativeGameplayTag(FName("CombatSocket.RightHand"), FString("Combat Socket RigntHand"));

	/* Montage Tags */
	GameplayTags.Event_Montage_GroundSword = Manager.AddNativeGameplayTag(FName("Event.Montage.GroundSword"), FString("GroundSword Montage"));

	/* Player Block Tags */
	GameplayTags.Player_Block_InputPressed = Manager.AddNativeGameplayTag(FName("Player.Block.InputPressed"), FString("Block Input Pressed callback for input"));
	GameplayTags.Player_Block_InputHeld = Manager.AddNativeGameplayTag(FName("Player.Block.Held"), FString("Block Input Held callback for input"));
	GameplayTags.Player_Block_InputReleased = Manager.AddNativeGameplayTag(FName("Player.Block.Released"), FString("Block Input Released callback for input"));

	GameplayTags.Damage = Manager.AddNativeGameplayTag(FName("Damage"), FString("Damage"));
	GameplayTags.Effects_HitReact = Manager.AddNativeGameplayTag(FName("Effects.HitReact"), FString("HitReact"));
}