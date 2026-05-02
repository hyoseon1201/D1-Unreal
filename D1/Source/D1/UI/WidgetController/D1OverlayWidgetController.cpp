
#include "UI/WidgetController/D1OverlayWidgetController.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include <Player/D1PlayerState.h>
#include "AbilitySystem/Data/D1LevelupInfo.h"
#include "GameplayEffect.h"

void UD1OverlayWidgetController::BroadcastInitialValues()
{
	OnHealthChanged.Broadcast(GetD1AS()->GetHealth());
	OnMaxHealthChanged.Broadcast(GetD1AS()->GetMaxHealth());
	OnManaChanged.Broadcast(GetD1AS()->GetMana());
	OnMaxManaChanged.Broadcast(GetD1AS()->GetMaxMana());
}

void UD1OverlayWidgetController::BindCallbacksToDependencies()
{
	GetD1PS()->OnXPChangedDelegate.AddUObject(this, &UD1OverlayWidgetController::OnXPChanged);
	GetD1PS()->OnLevelChangedDelegate.AddLambda(
		[this](int32 NewLevel)
		{
			OnPlayerLevelChangedDelegate.Broadcast(NewLevel);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnHealthChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnManaChanged.Broadcast(Data.NewValue);
		}
	);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetD1AS()->GetMaxManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxManaChanged.Broadcast(Data.NewValue);
		}
	);

	if (GetD1ASC())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BindCallbacks] bStartupAbilitiesGiven is: %s"), GetD1ASC()->bStartupAbilitiesGiven ? TEXT("TRUE") : TEXT("FALSE"));
		if (GetD1ASC()->bStartupAbilitiesGiven)
		{
				BroadcastAbilityInfo();
		}
		else
		{
			GetD1ASC()->AbilitiesGivenDelegate.AddUObject(this, &UD1OverlayWidgetController::BroadcastAbilityInfo);
		}
	}

	// AbilityStatusChanged: 어빌리티 상태/장착 변경 시 HUD에 반영
	GetD1ASC()->AbilityStatusChanged.AddLambda([this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, const FGameplayTag& InputTag, int32 NewLevel)
		{
			if (AbilityInfo)
			{
				// 변경된 상태 정보를 데이터에서 가져와 실시간 상태로 덮어씀
				FD1AbilityTagInfo Info = AbilityInfo->FindAbilityTagInforTag(AbilityTag);
				Info.StatusTag = StatusTag;
				Info.InputTag = InputTag;
				Info.Level = NewLevel;

				// HUD에 표시되는 모든 스킬 슬롯 위젯에 전송
				AbilityInfoDelegate.Broadcast(Info);

				if (InputTag.IsValid())
				{
					OnAbilityAssigned(AbilityTag, Info.CooldownTag);
				}
			}
		});
}

void UD1OverlayWidgetController::OnXPChanged(int32 NewXP)
{
	const UD1LevelupInfo* LevelUpInfo = GetD1PS()->LevelUpInfo;
	checkf(LevelUpInfo, TEXT("Unabled to find LevelUpInfo. Please fill out AuraPlayerState Blueprint"));

	const int32 Level = LevelUpInfo->FindLevelForXP(NewXP);
	const int32 MaxLevel = LevelUpInfo->LevelupInformation.Num();

	if (Level <= MaxLevel && Level > 0)
	{
		const int32 LevelUpRequirement = LevelUpInfo->LevelupInformation[Level].LevelupRequirement;
		const int32 PreviousLevelUpRequirement = LevelUpInfo->LevelupInformation[Level - 1].LevelupRequirement;

		const int32 DeltaLevelRequirement = LevelUpRequirement - PreviousLevelUpRequirement;
		const int32 XPForThisLevel = NewXP - PreviousLevelUpRequirement;

		const float XPBarPercent = static_cast<float>(XPForThisLevel) / static_cast<float>(DeltaLevelRequirement);

		OnXPPercentChangedDelegate.Broadcast(XPBarPercent);
	}
}

void UD1OverlayWidgetController::OnAbilityAssigned(const FGameplayTag& AbilityTag, const FGameplayTag& CooldownTag)
{
	AbilitySystemComponent->RegisterGameplayTagEvent(
		CooldownTag,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UD1OverlayWidgetController::OnCooldownTagChanged, AbilityTag);
}

void UD1OverlayWidgetController::OnCooldownTagChanged(const FGameplayTag CooldownTag, int32 NewCount, FGameplayTag AbilityTag)
{
	if (NewCount > 0)
	{
		FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTag.GetSingleTagContainer());

		// GetActiveEffectsTimeRemainingAndDuration: TTuple<Remaining, Duration> 반환
		auto Times = AbilitySystemComponent->GetActiveEffectsTimeRemainingAndDuration(Query);

		if (Times.Num() > 0)
		{
			float TimeRemaining = Times[0].Key;
			float Duration = Times[0].Value;
			for (int32 i = 1; i < Times.Num(); i++)
			{
				if (Times[i].Key > TimeRemaining)
				{
					TimeRemaining = Times[i].Key;
					Duration = Times[i].Value;
				}
			}

			float CooldownPercent = (Duration > 0.f) ? (TimeRemaining / Duration) : 0.f;

			OnCooldownTagChangedDelegate.Broadcast(AbilityTag, CooldownTag, CooldownPercent, Duration);
		}
	}
	else
	{
		OnCooldownTagChangedDelegate.Broadcast(AbilityTag, CooldownTag, 0.f, 0.f);
	}
}
