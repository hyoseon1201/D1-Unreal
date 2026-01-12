// Fill out your copyright notice in the Description page of Project Settings.


#include "D1AssetManager.h"

#include "D1GameplayTags.h"

UD1AssetManager& UD1AssetManager::Get()
{
	check(GEngine);

	UD1AssetManager* D1AssetManager =  Cast<UD1AssetManager>(GEngine->AssetManager);
	return *D1AssetManager;
}

void UD1AssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	FD1GameplayTags::InitializeNativeGameplayTags();
}
