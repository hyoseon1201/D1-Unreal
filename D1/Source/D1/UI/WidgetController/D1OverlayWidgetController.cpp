
#include "UI/WidgetController/D1OverlayWidgetController.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include <Player/D1PlayerState.h>
#include "AbilitySystem/Data/D1LevelupInfo.h"

void UD1OverlayWidgetController::BroadcastInitialValues()
{

	OnHealthChanged.Broadcast(GetD1AS()->GetHealth());
	OnMaxHealthChanged.Broadcast(GetD1AS()->GetMaxHealth());
	OnManaChanged.Broadcast(GetD1AS()->GetMana());
	OnMaxManaChanged.Broadcast(GetD1AS()->GetMaxMana());
}

void UD1OverlayWidgetController::BindCallbacksToDependencies()
{
	GetD1PS()->OnXPChangedDelegate.AddUObject(this, &UD1OverlayWidgetController::OnXPChanged);
	GetD1PS()->OnLevelChangedDelegate.AddLambda(
		[this](int32 NewLevel)
		{
			OnPlayerLevelChangedDelegate.Broadcast(NewLevel);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnHealthChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnManaChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetMaxManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxManaChanged.Broadcast(Data.NewValue);
		}
	);

	if (GetD1ASC())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BindCallbacks] bStartupAbilitiesGiven is: %s"), GetD1ASC()->bStartupAbilitiesGiven ? TEXT("TRUE") : TEXT("FALSE"));
		if (GetD1ASC()->bStartupAbilitiesGiven)
		{
				BroadcastAbilityInfo();
		}
		else
		{
			GetD1ASC()->AbilitiesGivenDelegate.AddUObject(this, &UD1OverlayWidgetController::BroadcastAbilityInfo);
		}
	}
}

void UD1OverlayWidgetController::OnXPChanged(int32 NewXP)
{
	const UD1LevelupInfo* LevelUpInfo = GetD1PS()->LevelUpInfo;
	checkf(LevelUpInfo, TEXT("Unabled to find LevelUpInfo. Please fill out AuraPlayerState Blueprint"));

	const int32 Level = LevelUpInfo->FindLevelForXP(NewXP);
	const int32 MaxLevel = LevelUpInfo->LevelupInformation.Num();

	if (Level <= MaxLevel && Level > 0)
	{
		const int32 LevelUpRequirement = LevelUpInfo->LevelupInformation[Level].LevelupRequirement;
		const int32 PreviousLevelUpRequirement = LevelUpInfo->LevelupInformation[Level - 1].LevelupRequirement;

		const int32 DeltaLevelRequirement = LevelUpRequirement - PreviousLevelUpRequirement;
		const int32 XPForThisLevel = NewXP - PreviousLevelUpRequirement;

		const float XPBarPercent = static_cast<float>(XPForThisLevel) / static_cast<float>(DeltaLevelRequirement);

		OnXPPercentChangedDelegate.Broadcast(XPBarPercent);
	}
}
