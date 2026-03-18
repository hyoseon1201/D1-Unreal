// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/D1SkillMenuWidgetController.h"

#include "AbilitySystem/D1AbilitySystemComponent.h"

void UD1SkillMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();
}

void UD1SkillMenuWidgetController::BindCallbacksToDependencies()
{
    if (GetD1ASC())
    {
        if (GetD1ASC()->bStartupAbilitiesGiven)
        {
            BroadcastAbilityInfo();
        }
        else
        {
            GetD1ASC()->AbilitiesGivenDelegate.AddUObject(this, &UD1SkillMenuWidgetController::BroadcastAbilityInfo);
        }
    }
}
