// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/D1WidgetController.h"

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
