// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/D1LevelupInfo.h"

int32 UD1LevelupInfo::FindLevelForXP(int32 XP) const
{
	int32 Level = 1;
	bool bSearching = true;
	while (bSearching)
	{
		// LevelUpInformation[1] = Level 1 Information
		// LevelUpInformation[2] = Level 1 Information
		if (LevelupInformation.Num() - 1 <= Level) return Level;

		if (XP >= LevelupInformation[Level].LevelupRequirement)
		{
			++Level;
		}
		else
		{
			bSearching = false;
		}
	}
	return Level;
}
