// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1GameInstance.h"

void UD1GameInstance::SavePlayerStateData(
	int32 InAttributePoints, int32 InLevel, int32 InXP,
	float InStrength, float InIntelligence,
	float InDexterity, float InLuck)
{
	SavedAttributePoints = InAttributePoints;
	SavedLevel = InLevel;
	SavedXP = InXP;
	SavedStrength = InStrength;
	SavedIntelligence = InIntelligence;
	SavedDexterity = InDexterity;
	SavedLuck = InLuck;
	bHasSavedData = true;

	UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] GameInstance Save. AttrPts=%d, Level=%d, XP=%d, Str=%.1f, Int=%.1f, Dex=%.1f, Luc=%.1f"),
		SavedAttributePoints, SavedLevel, SavedXP,
		SavedStrength, SavedIntelligence, SavedDexterity, SavedLuck);
}

void UD1GameInstance::RestorePlayerStateData(
	int32& OutAttributePoints, int32& OutLevel, int32& OutXP,
	float& OutStrength, float& OutIntelligence,
	float& OutDexterity, float& OutLuck) const
{
	if (bHasSavedData)
	{
		OutAttributePoints = SavedAttributePoints;
		OutLevel = SavedLevel;
		OutXP = SavedXP;
		OutStrength = SavedStrength;
		OutIntelligence = SavedIntelligence;
		OutDexterity = SavedDexterity;
		OutLuck = SavedLuck;

		UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] GameInstance Restore. AttrPts=%d, Level=%d, XP=%d, Str=%.1f, Int=%.1f, Dex=%.1f, Luc=%.1f"),
			OutAttributePoints, OutLevel, OutXP,
			OutStrength, OutIntelligence, OutDexterity, OutLuck);
	}
	else
	{
		OutAttributePoints = -1;
		OutLevel = -1;
		OutXP = -1;
		OutStrength = -1.f;
		OutIntelligence = -1.f;
		OutDexterity = -1.f;
		OutLuck = -1.f;

		UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] GameInstance Restore skipped. No saved data."));
	}
}

void UD1GameInstance::ClearSavedData()
{
	bHasSavedData = false;
	SavedAttributePoints = -1;
	SavedLevel = -1;
	SavedXP = -1;
	SavedStrength = -1.f;
	SavedIntelligence = -1.f;
	SavedDexterity = -1.f;
	SavedLuck = -1.f;
}
