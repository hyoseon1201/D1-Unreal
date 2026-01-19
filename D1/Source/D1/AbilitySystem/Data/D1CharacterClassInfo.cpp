// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/D1CharacterClassInfo.h"

FCharacterClassDefaultInfo UD1CharacterClassInfo::GetClassDefaultInfo(ECharacterClass CharacterClass)
{
	return CharacterClassInformation.FindChecked(CharacterClass);
}
