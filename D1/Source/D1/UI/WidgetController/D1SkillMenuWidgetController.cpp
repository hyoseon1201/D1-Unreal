// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/D1SkillMenuWidgetController.h"

#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Data/D1AbilityInfo.h"
#include "BlueprintGameplayTagLibrary.h"

void UD1SkillMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();
}

void UD1SkillMenuWidgetController::BindCallbacksToDependencies()
{

}

TArray<FD1AbilityTagInfo> UD1SkillMenuWidgetController::GetFilteredAbilityInfo()
{
    TArray<FD1AbilityTagInfo> FilteredInfo;
    if (!AbilityInfo || !AbilitySystemComponent) return FilteredInfo;

    AActor* AvatarActor = AbilitySystemComponent->GetAvatarActor();
    if (!AvatarActor) return FilteredInfo;

    // 디버깅: 실제 액터가 가진 Actor Tags 목록 출력
    for (const FName& Tag : AvatarActor->Tags)
    {
        UE_LOG(LogTemp, Log, TEXT("Avatar Actor Tag: %s"), *Tag.ToString());
    }

    for (const FD1AbilityTagInfo& Info : AbilityInfo->AbilityInformation)
    {
        // 1. 공용 스킬 체크
        if (!Info.ClassTag.IsValid())
        {
            FilteredInfo.Add(Info);
            continue;
        }

        // 2. GameplayTag를 FName으로 변환하여 Actor Tag와 비교
        // Info.ClassTag가 "Class.Warrior"라면 "Warrior" 부분만 추출하거나 전체 이름을 비교해야 합니다.
        // 여기서는 가장 확실하게 태그의 'Leaf Name'(마지막 이름) 혹은 전체 이름을 FName으로 변환해 체크합니다.

        FName TargetTagName = Info.ClassTag.GetTagName(); // "Class.Warrior" 전체 이름

        // 만약 액터 태그에 "Warrior"라고만 적었다면, 아래와 같이 비교 로직이 필요할 수 있습니다.
        // 우선은 정확한 매칭을 위해 ActorHasTag를 사용합니다.
        if (AvatarActor->ActorHasTag(TargetTagName) || AvatarActor->ActorHasTag(FName("Warrior")))
        {
            UE_LOG(LogTemp, Log, TEXT("Match Success: %s has tag %s"), *AvatarActor->GetName(), *TargetTagName.ToString());
            FilteredInfo.Add(Info);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Match Failed: %s does not have tag %s"), *AvatarActor->GetName(), *TargetTagName.ToString());
        }
    }

    return FilteredInfo;
}