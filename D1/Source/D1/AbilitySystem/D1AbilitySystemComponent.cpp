// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/D1AbilitySystemComponent.h"

#include "D1GameplayTags.h"
#include "AbilitySystem/Ability/D1GameplayAbility.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Data/D1AbilityInfo.h"

void UD1AbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UD1AbilitySystemComponent::ClientEffectApplied);
}

void UD1AbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		if (const UD1GameplayAbility* D1Ability = Cast<UD1GameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(D1Ability->StartupInputTag);
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(FD1GameplayTags::Get().Abilities_Status_Locked);
			GiveAbility(AbilitySpec);
		}
	}

	bStartupAbilitiesGiven = true;

	ENetRole Role = GetOwnerRole();
	UE_LOG(LogTemp, Warning, TEXT("[AddCharacterAbilities] Role: %d, bStartupAbilitiesGiven set to TRUE"), Role);

	AbilitiesGivenDelegate.Broadcast();
}

void UD1AbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

void UD1AbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);
			if (AbilitySpec.IsActive())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
			}
		}
	}
}

void UD1AbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);
			if (!AbilitySpec.IsActive())
			{
				TryActivateAbility(AbilitySpec.Handle);
			}
		}
	}
}

void UD1AbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) && AbilitySpec.IsActive())
		{
			AbilitySpecInputReleased(AbilitySpec);
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
		}
	}
}

void UD1AbilitySystemComponent::ForEachAbility(const FForEachAbility& Delegate)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!Delegate.ExecuteIfBound(AbilitySpec))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to execute delegate in %hs"), __FUNCTION__);
		}
	}
}

FGameplayTag UD1AbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability)
	{
		// 1. 어떤 어빌리티를 검사하고 있는지 출력
		UE_LOG(LogTemp, Warning, TEXT("[GetAbilityTag] Checking Ability: %s"), *AbilitySpec.Ability->GetName());

		for (FGameplayTag Tag : AbilitySpec.Ability.Get()->GetAssetTags())
		{
			// 2. 이 어빌리티가 무슨 태그들을 가지고 있는지 전부 출력
			UE_LOG(LogTemp, Warning, TEXT("   - Found Tag in Ability: [%s]"), *Tag.ToString());

			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities"))))
			{
				return Tag;
			}
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[GetAbilityTag] Failed to find matching tag for %s!"), *GetNameSafe(AbilitySpec.Ability.Get()));
	return FGameplayTag();
}

FGameplayTag UD1AbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (FGameplayTag Tag : AbilitySpec.GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("InputTag"))))
		{
			return Tag;
		}
	}

	return FGameplayTag();
}

FGameplayTag UD1AbilitySystemComponent::GetStatusFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (FGameplayTag StatusTag : AbilitySpec.GetDynamicSpecSourceTags())
	{
		if (StatusTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Status"))))
		{
			return StatusTag;
		}
	}

	return FGameplayTag();
}

FGameplayAbilitySpec* UD1AbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag& AbilityTag)
{
	FScopedAbilityListLock ActivateScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		for (FGameplayTag Tag : AbilitySpec.Ability.Get()->GetAssetTags())
		{
			if (Tag.MatchesTag(AbilityTag))
			{
				return &AbilitySpec;
			}
		}
	}

	return nullptr;
}

void UD1AbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
		{
			ServerUpgradeAttribute(AttributeTag);
		}
	}
}

void UD1AbilitySystemComponent::UpdateAbilityStatuses(int32 Level)
{
	UD1AbilityInfo* AbilityInfo = UD1AbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
	if (!AbilityInfo)
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateAbilityStatuses: AbilityInfo is NULL!"));
		return;
	}

	FGameplayTag MyClassTag;
	if (IPlayerInterface* PlayerInt = Cast<IPlayerInterface>(GetAvatarActor()))
	{
		MyClassTag = PlayerInt->Execute_GetCharacterClassTag(GetAvatarActor());
		UE_LOG(LogTemp, Log, TEXT("UpdateAbilityStatuses: MyClassTag is [%s]"), *MyClassTag.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateAbilityStatuses: PlayerInterface NOT FOUND on Avatar!"));
	}

	UE_LOG(LogTemp, Log, TEXT("UpdateAbilityStatuses: Checking abilities for Level [%d]"), Level);

	for (const FD1AbilityTagInfo& Info : AbilityInfo->AbilityInformation)
	{
		if (!Info.AbilityTag.IsValid()) continue;
		if (Info.ClassTag.IsValid() && !MyClassTag.MatchesTag(Info.ClassTag)) continue;
		if (Level < Info.LevelRequirement) continue;

		// 1. 해당 태그를 가진 스펙이 이미 있는지 확인
		FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(Info.AbilityTag);

		if (AbilitySpec == nullptr)
		{
			// [신규 부여] 아직 없는 스킬인 경우 (보통 새로운 레벨대에 진입했을 때)
			FGameplayAbilitySpec NewSpec = FGameplayAbilitySpec(Info.Ability, 1);
			NewSpec.GetDynamicSpecSourceTags().AddTag(FD1GameplayTags::Get().Abilities_Status_Eligible);
			GiveAbility(NewSpec);
			MarkAbilitySpecDirty(NewSpec);

			UE_LOG(LogTemp, Log, TEXT(">>> GIVING NEW ABILITY: [%s]"), *Info.AbilityTag.ToString());
		}
		else
		{
			// [기존 업데이트] 이미 스펙은 있지만, 상태 태그가 업데이트되어야 하는 경우 (예: Locked -> Eligible)
			FGameplayTag EligibleTag = FD1GameplayTags::Get().Abilities_Status_Eligible;
			FGameplayTag LockedTag = FD1GameplayTags::Get().Abilities_Status_Locked;

			if (AbilitySpec->GetDynamicSpecSourceTags().HasTag(LockedTag))
			{
				AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(LockedTag);
				AbilitySpec->GetDynamicSpecSourceTags().AddTag(EligibleTag);

				// 변경 사항을 클라이언트와 UI에 알림
				MarkAbilitySpecDirty(*AbilitySpec);

				UE_LOG(LogTemp, Log, TEXT(">>> UPDATING EXISTING ABILITY: [%s] to Eligible"), *Info.AbilityTag.ToString());
			}
		}
	}
}

void UD1AbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag& AttributeTag)
{
	FGameplayEventData Payload;
	Payload.EventTag = AttributeTag;
	Payload.EventMagnitude = 1.f;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), AttributeTag, Payload);

	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(), -1);
	}
}

void UD1AbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	if (!bStartupAbilitiesGiven)
	{
		bStartupAbilitiesGiven = true;
		AbilitiesGivenDelegate.Broadcast();
	}
}

void UD1AbilitySystemComponent::ClientUpdateAbilityStatus_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag)
{
	AbilityStatusChanged.Broadcast(AbilityTag, StatusTag);
}

void UD1AbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);

	EffectAssetTags.Broadcast(TagContainer);
}
