// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include "D1SkillMenuWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSelectedAbilityChangedSignature, const FGameplayTag&, AbilityTag, const FGameplayTag&, StatusTag, int32, AbilityLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipModeChangedSignature, bool, bIsEquipMode);

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

	UPROPERTY(BlueprintAssignable)
	FOnEquipModeChangedSignature OnEquipModeChanged;

	UFUNCTION(BlueprintCallable)
	void AbilitySelected(const FGameplayTag& AbilityTag);

	UFUNCTION(BlueprintCallable, Category = "GAS|Selection")
	FD1AbilityTagInfo FindAbilityInfoForTag(const FGameplayTag& AbilityTag) const;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void LearnSkill(const FGameplayTag& SkillTag);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void EquipSkill(const FGameplayTag& SkillTag, const FGameplayTag& SlotTag);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void LevelUpSkill(const FGameplayTag& SkillTag);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SetEquipMode(bool bIsEquipMode);

	UFUNCTION(BlueprintPure, Category = "Skill")
	FGameplayTag GetSelectedAbilityTag() const { return SelectedAbilityTag; }

protected:
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	TArray<FD1AbilityTagInfo> GetFilteredAbilityInfo();

	FGameplayTag SelectedAbilityTag;
};
