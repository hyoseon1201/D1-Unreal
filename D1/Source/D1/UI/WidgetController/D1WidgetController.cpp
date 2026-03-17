// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/D1WidgetController.h"

#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"

void UD1WidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	PlayerController = WCParams.PlayerController;
	PlayerState = WCParams.PlayerState;
	AbilitySystemComponent = WCParams.AbilitySystemComponent;
	AttributeSet = WCParams.AttributeSet;
}

void UD1WidgetController::BroadcastInitialValues()
{
}

void UD1WidgetController::BindCallbacksToDependencies()
{
}

void UD1WidgetController::BroadcastAbilityInfo()
{
	if (!GetD1ASC()->bStartupAbilitiesGiven) return;

	FForEachAbility BroadcastDelegate;
	BroadcastDelegate.BindLambda([this](const FGameplayAbilitySpec& AbilitySpec)
		{
			FD1AbilityTagInfo Info = AbilityInfo->FindAbilityTagInforTag(GetD1ASC()->GetAbilityTagFromSpec(AbilitySpec));
			if (Info.AbilityTag.IsValid())
			{
				Info.InputTag = GetD1ASC()->GetInputTagFromSpec(AbilitySpec);
				AbilityInfoDelegate.Broadcast(Info);
			}
		});
	GetD1ASC()->ForEachAbility(BroadcastDelegate);
}

AD1PlayerController* UD1WidgetController::GetD1PC()
{
	if (D1PlayerController == nullptr)
	{
		D1PlayerController = Cast<AD1PlayerController>(PlayerController);
	}

	return D1PlayerController;
}

AD1PlayerState* UD1WidgetController::GetD1PS()
{
	if (D1PlayerState == nullptr)
	{
		D1PlayerState = Cast<AD1PlayerState>(PlayerState);
	}

	return D1PlayerState;
}

UD1AbilitySystemComponent* UD1WidgetController::GetD1ASC()
{
	if (D1AbilitySystemComponent == nullptr)
	{
		D1AbilitySystemComponent = Cast<UD1AbilitySystemComponent>(AbilitySystemComponent);
	}
	return D1AbilitySystemComponent;
}

UD1AttributeSet* UD1WidgetController::GetD1AS()
{
	if (D1AttributeSet == nullptr)
	{
		D1AttributeSet = Cast<UD1AttributeSet>(AttributeSet);
	}
	return D1AttributeSet;
}
