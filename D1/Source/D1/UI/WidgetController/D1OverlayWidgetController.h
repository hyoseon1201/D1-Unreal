#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include "D1OverlayWidgetController.generated.h"

class UD1AbilityInfo;
class UD1AbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCooldownSignature, const FGameplayTag&, AbilityTag, const FGameplayTag&, CooldownTag, float, CooldownPercent, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeveledUpSignature, int32, NewLevel);  // 진짜 레벨업 전용 (복원/동기화 시 미발동)

UCLASS(BlueprintType, Blueprintable)
class D1_API UD1OverlayWidgetController : public UD1WidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnMaxManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|XP")
	FOnAttributeChangedSignature OnXPPercentChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Level")
	FOnPlayerStatChangedSignature OnPlayerLevelChangedDelegate;

	/** XP 획득으로 레벨이 실제로 오를 때만 발동. 복원/맵이동 동기화 시엔 발동 안 함. */
	UPROPERTY(BlueprintAssignable, Category = "GAS|Level")
	FOnPlayerLeveledUpSignature OnPlayerLeveledUpDelegate;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Cooldown")
	FOnCooldownSignature OnCooldownTagChangedDelegate;

protected:

	template<typename T>
	T* GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag);

	void OnXPChanged(int32 NewXP);

	void OnAbilityAssigned(const FGameplayTag& AbilityTag, const FGameplayTag& CooldownTag);

	void OnCooldownTagChanged(const FGameplayTag CooldownTag, int32 NewCount, FGameplayTag AbilityTag);
};

template <typename T>
T* UD1OverlayWidgetController::GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag)
{
	return DataTable->FindRow<T>(Tag.GetTagName(), TEXT(""));
}