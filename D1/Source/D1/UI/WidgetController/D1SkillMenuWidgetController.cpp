// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/D1SkillMenuWidgetController.h"

#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include "BlueprintGameplayTagLibrary.h"
#include "Characters/D1CharacterBase.h"
#include "Interaction/PlayerInterface.h"
#include "Player/D1PlayerState.h"
#include "D1GameplayTags.h"

void UD1SkillMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();
    SkillPointsChanged.Broadcast(GetD1PS()->GetSkillPoints());
}

void UD1SkillMenuWidgetController::BindCallbacksToDependencies()
{
    // 1. [데이터 변경 시점] 서버에서 스킬 상태/장착 정보가 변했을 때 호출됨
    GetD1ASC()->AbilityStatusChanged.AddLambda([this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, const FGameplayTag& InputTag, int32 NewLevel)
        {
            if (AbilityInfo)
            {
                // [중요] 개별 스킬 정보 갱신 (스킬 목록, 장착 슬롯, HUD용)
                FD1AbilityTagInfo Info = AbilityInfo->FindAbilityTagInforTag(AbilityTag);
                Info.StatusTag = StatusTag;
                Info.InputTag = InputTag;
                Info.Level = NewLevel;

                AbilityInfoDelegate.Broadcast(Info);

                // 선택된 스킬 상세 정보창 갱신
                if (AbilityTag.MatchesTagExact(SelectedAbilityTag))
                {
                    SelectedAbilityChangedDelegate.Broadcast(AbilityTag, StatusTag, NewLevel);
                }
            }
        });

    // 2. [스킬 포인트 리스너] - 그대로 유지
    GetD1PS()->OnSkillPointsChangedDelegate.AddLambda([this](int32 SkillPoints)
        {
            SkillPointsChanged.Broadcast(SkillPoints);
        });

    // 3. [초기화] 위젯이 켜지는 순간의 데이터 스냅샷 전송
    SkillPointsChanged.Broadcast(GetD1PS()->GetSkillPoints());
    BroadcastAbilityInfo();
}

void UD1SkillMenuWidgetController::AbilitySelected(const FGameplayTag& AbilityTag)
{
    if (!GetD1ASC()) return;

    FGameplayTag StatusTag;
    int32 AbilityLevel = 0;

    FGameplayAbilitySpec* Spec = GetD1ASC()->GetSpecFromAbilityTag(AbilityTag);

    if (Spec)
    {
        // Spec을 찾은 경우
        StatusTag = GetD1ASC()->GetStatusFromSpec(*Spec);
        AbilityLevel = Spec->Level;
    }
    else
    {
        // Spec을 못 찾은 경우 (Locked 처리)
        StatusTag = FD1GameplayTags::Get().Abilities_Status_Locked;
    }

    SelectedAbilityTag = AbilityTag;
    SelectedAbilityChangedDelegate.Broadcast(AbilityTag, StatusTag, AbilityLevel);
}

FD1AbilityTagInfo UD1SkillMenuWidgetController::FindAbilityInfoForTag(const FGameplayTag& AbilityTag) const
{
    if (AbilityInfo)
    {
        return AbilityInfo->FindAbilityTagInforTag(AbilityTag);
    }
    return FD1AbilityTagInfo();
}

void UD1SkillMenuWidgetController::LearnSkill(const FGameplayTag& SkillTag)
{
    if (GetD1ASC())
    {
        GetD1ASC()->ServerUpgradeAbility(SkillTag);
    }
}

void UD1SkillMenuWidgetController::EquipSkill(const FGameplayTag& SkillTag, const FGameplayTag& SlotTag)
{
    if (GetD1ASC())
    {
        GetD1ASC()->ServerEquipAbility(SkillTag, SlotTag);
    }
}

void UD1SkillMenuWidgetController::LevelUpSkill(const FGameplayTag& SkillTag)
{
    if (GetD1ASC())
    {
        GetD1ASC()->ServerUpgradeAbility(SkillTag);
    }
}

void UD1SkillMenuWidgetController::SetEquipMode(bool bIsEquipMode)
{
    OnEquipModeChanged.Broadcast(bIsEquipMode);
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
