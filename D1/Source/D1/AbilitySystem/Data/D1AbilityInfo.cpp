// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/D1AbilityInfo.h"

FD1AbilityTagInfo UD1AbilityInfo::FindAbilityTagInforTag(const FGameplayTag& AbilityTag, bool bLogNotFound) const
{
	for (const FD1AbilityTagInfo& Info : AbilityInformation)
	{
		if (Info.AttributeTag == AbilityTag)
		{
			return Info;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find info for AbilityTag [%s] on AbilityInfo [%s]"), *AbilityTag.ToString(), *GetNameSafe(this));
	}

	return FD1AbilityTagInfo();
}
