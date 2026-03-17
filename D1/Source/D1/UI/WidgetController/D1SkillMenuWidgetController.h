// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "D1SkillMenuWidgetController.generated.h"

/**
 * 
 */
UCLASS()
class D1_API UD1SkillMenuWidgetController : public UD1WidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
};
