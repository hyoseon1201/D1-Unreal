
#include "UI/WidgetController/D1OverlayWidgetController.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystemComponent.h"

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
}