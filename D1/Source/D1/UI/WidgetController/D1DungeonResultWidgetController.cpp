// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1DungeonResultWidgetController.h"
#include "Player/D1PlayerController.h"

void UD1DungeonResultWidgetController::BroadcastInitialValues()
{
	OnAcquiredItemsChanged.Broadcast(AcquiredItems);
}

void UD1DungeonResultWidgetController::SetAcquiredItems(const TArray<FText>& InItems)
{
	AcquiredItems = InItems;
	OnAcquiredItemsChanged.Broadcast(AcquiredItems);
}

void UD1DungeonResultWidgetController::ReturnToTown()
{
	if (AD1PlayerController* PC = Cast<AD1PlayerController>(PlayerController))
	{
		PC->Server_ReturnToTown();
	}
}
