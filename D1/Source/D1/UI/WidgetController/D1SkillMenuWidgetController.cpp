// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/D1SkillMenuWidgetController.h"

#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include "BlueprintGameplayTagLibrary.h"
#include "Characters/D1CharacterBase.h"
#include "Interaction/PlayerInterface.h"
#include "Player/D1PlayerState.h"

void UD1SkillMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();
    SkillPointsChanged.Broadcast(GetD1PS()->GetSkillPoints());
}

void UD1SkillMenuWidgetController::BindCallbacksToDependencies()
{
    GetD1ASC()->AbilityStatusChanged.AddLambda([this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag)
        {
            if (AbilityInfo)
            {
                FD1AbilityTagInfo Info = AbilityInfo->FindAbilityTagInforTag(AbilityTag);
                Info.StatusTag = StatusTag;
                AbilityInfoDelegate.Broadcast(Info);
            }
        });

    GetD1PS()->OnSkillPointsChangedDelegate.AddLambda([this](int32 SkillPoints)
        {
            SkillPointsChanged.Broadcast(SkillPoints);
        });
}

TArray<FD1AbilityTagInfo> UD1SkillMenuWidgetController::GetFilteredAbilityInfo()
{
    TArray<FD1AbilityTagInfo> FilteredInfo;
    if (!AbilityInfo || !AbilitySystemComponent) return FilteredInfo;

    // 1. 아바타 액터를 가져옵니다.
    AActor* AvatarActor = AbilitySystemComponent->GetAvatarActor();
    if (!AvatarActor) return FilteredInfo;

    // 2. 인터페이스를 통해 클래스 태그를 가져옵니다.
    FGameplayTag MyClassTag;
    if (IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(AvatarActor))
    {
        // BlueprintNativeEvent이므로 Execute_ 함수명을 사용합니다.
        MyClassTag = IPlayerInterface::Execute_GetCharacterClassTag(AvatarActor);
    }

    // 3. 필터링 로직
    for (const FD1AbilityTagInfo& Info : AbilityInfo->AbilityInformation)
    {
        // 공용 스킬(태그가 비어있음)이거나, 내 클래스 태그와 일치하는 경우
        if (!Info.ClassTag.IsValid() || MyClassTag.MatchesTag(Info.ClassTag))
        {
            FilteredInfo.Add(Info);
        }
    }

    return FilteredInfo;
}