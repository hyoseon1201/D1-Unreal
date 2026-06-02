// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/D1DungeonResultWidgetController.h"

void UD1DungeonResultWidgetController::BroadcastInitialValues()
{
	UE_LOG(LogTemp, Warning, TEXT("[DungeonUI] WC::BroadcastInitialValues called. Items=%d"), AcquiredItems.Num());
	OnAcquiredItemsChanged.Broadcast(AcquiredItems);
}

void UD1DungeonResultWidgetController::SetAcquiredItems(const TArray<FText>& InItems)
{
	AcquiredItems = InItems;
	UE_LOG(LogTemp, Warning, TEXT("[DungeonUI] WC::SetAcquiredItems called. Items=%d. Broadcasting delegate..."), AcquiredItems.Num());
	OnAcquiredItemsChanged.Broadcast(AcquiredItems);
	UE_LOG(LogTemp, Warning, TEXT("[DungeonUI] WC::SetAcquiredItems Broadcast DONE."));
}
