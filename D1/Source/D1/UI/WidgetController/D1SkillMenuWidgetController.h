// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "D1SkillMenuWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSelectedAbilityChangedSignature, const FGameplayTag&, AbilityTag, const FGameplayTag&, StatusTag);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1SkillMenuWidgetController : public UD1WidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerStatChangedSignature SkillPointsChanged;

	UPROPERTY(BlueprintAssignable)
	FSelectedAbilityChangedSignature SelectedAbilityChangedDelegate;

	UFUNCTION(BlueprintCallable)
	void AbilitySelected(const FGameplayTag& AbilityTag);

protected:
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	TArray<FD1AbilityTagInfo> GetFilteredAbilityInfo();
};
