
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

	for (auto& Pair : AS->TagsToAttributes)
	{
		BroadcastAttributeInfo(Pair.Key, Pair.Value());
	}
}

void UD1AttributeMenuWidgetController::BindCallbacksToDependencies()
{
	UD1AttributeSet* AS = CastChecked<UD1AttributeSet>(AttributeSet);
	check(AttributeInfo);

	for (auto& Pair : AS->TagsToAttributes)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Value()).AddLambda(
			[this, Pair](const FOnAttributeChangeData& Data)
			{
				BroadcastAttributeInfo(Pair.Key, Pair.Value());
			}
		);
	}
}

void UD1AttributeMenuWidgetController::BroadcastAttributeInfo(const FGameplayTag& AttributeTag, const FGameplayAttribute& Attribute) const
{
	FD1AttributeTagInfo Info = AttributeInfo->FindAttributeInfoForTag(AttributeTag);
	Info.AttributeValue = Attribute.GetNumericValue(AttributeSet);
	AttributeInfoDelegate.Broadcast(Info);
}