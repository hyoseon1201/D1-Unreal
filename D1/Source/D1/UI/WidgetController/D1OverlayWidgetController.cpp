
#include "UI/WidgetController/D1OverlayWidgetController.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include <Player/D1PlayerState.h>
#include "AbilitySystem/Data/D1LevelupInfo.h"

void UD1OverlayWidgetController::BroadcastInitialValues()
{
	const UD1AttributeSet* D1AS = CastChecked<UD1AttributeSet>(AttributeSet);

	OnHealthChanged.Broadcast(D1AS->GetHealth());
	OnMaxHealthChanged.Broadcast(D1AS->GetMaxHealth());
	OnManaChanged.Broadcast(D1AS->GetMana());
	OnMaxManaChanged.Broadcast(D1AS->GetMaxMana());
}

void UD1OverlayWidgetController::BindCallbacksToDependencies()
{
	AD1PlayerState* D1PS = CastChecked<AD1PlayerState>(PlayerState);
	D1PS->OnXPChangedDelegate.AddUObject(this, &UD1OverlayWidgetController::OnXPChanged);
	D1PS->OnLevelChangedDelegate.AddLambda(
		[this](int32 NewLevel)
		{
			OnPlayerLevelChangedDelegate.Broadcast(NewLevel);
		}
	);

	const UD1AttributeSet* D1AS = CastChecked<UD1AttributeSet>(AttributeSet);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(D1AS->GetHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnHealthChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(D1AS->GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(D1AS->GetManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnManaChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(D1AS->GetMaxManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxManaChanged.Broadcast(Data.NewValue);
		}
	);

	if (UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(AbilitySystemComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BindCallbacks] bStartupAbilitiesGiven is: %s"), D1ASC->bStartupAbilitiesGiven ? TEXT("TRUE") : TEXT("FALSE"));
		if (D1ASC->bStartupAbilitiesGiven)
		{
			OnInitializeStartupAbilities(D1ASC);
		}
		else
		{
			D1ASC->AbilitiesGivenDelegate.AddUObject(this, &UD1OverlayWidgetController::OnInitializeStartupAbilities);
		}
	}
}

void UD1OverlayWidgetController::OnInitializeStartupAbilities(UD1AbilitySystemComponent* D1ASC)
{
	if (!D1ASC->bStartupAbilitiesGiven) return;

	FForEachAbility BroadcastDelegate;
	BroadcastDelegate.BindLambda([this, D1ASC](const FGameplayAbilitySpec& AbilitySpec)
		{
			FD1AbilityTagInfo Info = AbilityInfo->FindAbilityTagInforTag(D1ASC->GetAbilityTagFromSpec(AbilitySpec));
			if (Info.AbilityTag.IsValid())
			{
				Info.InputTag = D1ASC->GetInputTagFromSpec(AbilitySpec);
				AbilityInfoDelegate.Broadcast(Info);
			}
		});
	D1ASC->ForEachAbility(BroadcastDelegate);
}

void UD1OverlayWidgetController::OnXPChanged(int32 NewXP) const
{
	const AD1PlayerState* AuraPlayerState = CastChecked<AD1PlayerState>(PlayerState);
	const UD1LevelupInfo* LevelUpInfo = AuraPlayerState->LevelUpInfo;
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
