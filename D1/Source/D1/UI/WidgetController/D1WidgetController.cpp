// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/D1WidgetController.h"

#include "Player/D1PlayerController.h"
#include "Player/D1PlayerState.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include <D1GameplayTags.h>

void UD1WidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	PlayerController = WCParams.PlayerController;
	PlayerState = WCParams.PlayerState;
	AbilitySystemComponent = WCParams.AbilitySystemComponent;
	AttributeSet = WCParams.AttributeSet;
}

void UD1WidgetController::BroadcastInitialValues()
{
}

void UD1WidgetController::BindCallbacksToDependencies()
{
}

void UD1WidgetController::BroadcastAbilityInfo()
{
    if (!GetD1ASC() || !AbilityInfo) return;

    // 1. [스킬 리스트/HUD용] 개별 스킬 정보 방송 (전체 리스트 갱신)
    GetD1ASC()->ForEachAbility(FForEachAbility::CreateLambda([this](const FGameplayAbilitySpec& AbilitySpec)
        {
            FGameplayTag AbilityTag = GetD1ASC()->GetAbilityTagFromSpec(AbilitySpec);
            FD1AbilityTagInfo Info = AbilityInfo->FindAbilityTagInforTag(AbilityTag);

            if (Info.AbilityTag.IsValid())
            {
                // [체크!] GetInputTagFromSpec 내부가 MatchesTagExact를 쓰는지 확인 필수
                Info.InputTag = GetD1ASC()->GetInputTagFromSpec(AbilitySpec);
                Info.StatusTag = GetD1ASC()->GetStatusFromSpec(AbilitySpec);

                // 스킬 메뉴 리스트 위젯들이 이 방송을 듣습니다.
                AbilityInfoDelegate.Broadcast(Info);
            }
        }));
}

AD1PlayerController* UD1WidgetController::GetD1PC()
{
	if (D1PlayerController == nullptr)
	{
		D1PlayerController = Cast<AD1PlayerController>(PlayerController);
	}

	return D1PlayerController;
}

AD1PlayerState* UD1WidgetController::GetD1PS()
{
	if (D1PlayerState == nullptr)
	{
		D1PlayerState = Cast<AD1PlayerState>(PlayerState);
	}

	return D1PlayerState;
}

UD1AbilitySystemComponent* UD1WidgetController::GetD1ASC()
{
	if (D1AbilitySystemComponent == nullptr)
	{
		D1AbilitySystemComponent = Cast<UD1AbilitySystemComponent>(AbilitySystemComponent);
	}
	return D1AbilitySystemComponent;
}

UD1AttributeSet* UD1WidgetController::GetD1AS()
{
	if (D1AttributeSet == nullptr)
	{
		D1AttributeSet = Cast<UD1AttributeSet>(AttributeSet);
	}
	return D1AttributeSet;
}
