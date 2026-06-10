// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/D1AttributeInfo.h"
#include "D1/D1.h"

FD1AttributeTagInfo UD1AttributeInfo::FindAttributeInfoForTag(const FGameplayTag& AttributeTag, bool bLogNotFound) const
{
	for (const FD1AttributeTagInfo& Info : AttributeInformation)
	{
		if (Info.AttributeTag.MatchesTagExact(AttributeTag))
		{
			return Info;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogD1, Error, TEXT("Can't find Info for AttributeTag [%s] on AttributeInfo [%s]."), *AttributeTag.ToString(), *GetNameSafe(this));
	}

	return FD1AttributeTagInfo();
}