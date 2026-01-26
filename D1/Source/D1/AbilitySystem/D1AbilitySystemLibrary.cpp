// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/D1AbilitySystemLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "UI/HUD/D1HUD.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "Player/D1PlayerState.h"
#include "Game/D1GameModeBase.h"
#include "Interaction/CombatInterface.h"
#include "D1AbilityTypes.h"

UD1OverlayWidgetController* UD1AbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AD1HUD* D1HUD = Cast<AD1HUD>(PC->GetHUD()))
		{
			AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();
			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			return D1HUD->GetOverlayWidgetController(WidgetControllerParams);
		}
	}

	return nullptr;
}

UD1AttributeMenuWidgetController* UD1AbilitySystemLibrary::GetAttributeMenuWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AD1HUD* D1HUD = Cast<AD1HUD>(PC->GetHUD()))
		{
			AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();
			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			return D1HUD->GetAttributeMenuWidgetController(WidgetControllerParams);
		}
	}

	return nullptr;
}

void UD1AbilitySystemLibrary::InitializeDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	AActor* AvatarActor = ASC->GetAvatarActor();

	UD1CharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	FCharacterClassDefaultInfo ClassDefaultInfo = GetCharacterClassInfo(WorldContextObject)->GetClassDefaultInfo(CharacterClass);
	
	FGameplayEffectContextHandle PrimaryAttributesContextHandle = ASC->MakeEffectContext();
	PrimaryAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle PrimaryAttributesSpecHandle = ASC->MakeOutgoingSpec(ClassDefaultInfo.PrimaryAttribute, Level, PrimaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributesSpecHandle.Data.Get());

	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, Level, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	FGameplayEffectContextHandle VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes, Level, VitalAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());
}

void UD1AbilitySystemLibrary::GiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, ECharacterClass CharacterClass)
{
	AD1GameModeBase* D1GameMode = Cast<AD1GameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (D1GameMode == nullptr) return;

	UD1CharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	for (TSubclassOf<UGameplayAbility> AbilityClass : CharacterClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}
}

UD1CharacterClassInfo* UD1AbilitySystemLibrary::GetCharacterClassInfo(const UObject* WorldContextObject)
{
	AD1GameModeBase* D1GameMode = Cast<AD1GameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (D1GameMode == nullptr) return nullptr;
	return D1GameMode->CharacterClassInfo;
}

bool UD1AbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FD1GameplayEffectContext* D1EffectContext = static_cast<const FD1GameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return D1EffectContext->IsCriticalHit();
	}
	return false;
}

void UD1AbilitySystemLibrary::SetIsCriticalHit(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, bool bInIsCriticalHit)
{
	if (FD1GameplayEffectContext* D1EffectContext = static_cast<FD1GameplayEffectContext*>(EffectContextHandle.Get()))
	{
		D1EffectContext->SetIsCriticalHit(bInIsCriticalHit);
	}
}
