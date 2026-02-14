
#include "UI/WidgetController/D1OverlayWidgetController.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"

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
			Info.InputTag = D1ASC->GetInputTagFromSpec(AbilitySpec);
			AbilityInfoDelegate.Broadcast(Info);
		});
	D1ASC->ForEachAbility(BroadcastDelegate);
}
