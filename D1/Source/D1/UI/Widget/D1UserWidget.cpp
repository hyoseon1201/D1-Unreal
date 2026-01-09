// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/D1UserWidget.h"

void UD1UserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}
