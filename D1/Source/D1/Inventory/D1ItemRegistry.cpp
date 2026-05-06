// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/D1ItemRegistry.h"

UD1ItemData* UD1ItemRegistry::FindItemData(const FName& ItemID) const
{
	if (const TObjectPtr<UD1ItemData>* Found = Items.Find(ItemID))
	{
		return Found->Get();
	}
	return nullptr;
}
