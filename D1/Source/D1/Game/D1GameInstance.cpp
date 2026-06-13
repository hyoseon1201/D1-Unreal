// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameInstance.h"
#include "D1/D1.h"

void UD1GameInstance::SavePlayerData(const FString& PlayerId, const FD1SavedPlayerData& Data)
{
	SavedPlayers.Add(PlayerId, Data);

	UE_LOG(LogD1Travel, Warning,
		TEXT("GameInstance::SavePlayerData [%s]: Level=%d, AttrPts=%d, SkillPts=%d, Str=%.1f, Inventory=%d, Equipped=%d, Abilities=%d"),
		*PlayerId, Data.Level, Data.AttributePoints, Data.SkillPoints, Data.Strength,
		Data.InventorySlots.Num(), Data.EquippedItems.Num(), Data.AbilityStates.Num());
}

bool UD1GameInstance::TryGetPlayerData(const FString& PlayerId, FD1SavedPlayerData& OutData) const
{
	if (const FD1SavedPlayerData* Found = SavedPlayers.Find(PlayerId))
	{
		OutData = *Found;
		return true;
	}
	return false;
}

void UD1GameInstance::ClearPlayerData(const FString& PlayerId)
{
	SavedPlayers.Remove(PlayerId);
	UE_LOG(LogD1Travel, Warning, TEXT("GameInstance::ClearPlayerData [%s]"), *PlayerId);
}

void UD1GameInstance::ClearAllSavedData()
{
	SavedPlayers.Empty();
	UE_LOG(LogD1Travel, Warning, TEXT("GameInstance::ClearAllSavedData"));
}
