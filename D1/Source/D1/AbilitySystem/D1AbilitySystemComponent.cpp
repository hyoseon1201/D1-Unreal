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

FGameplayAbilitySpec* UD1AbilitySystemComponent::GetSpecFromInputTag(const FGameplayTag& InputTag)
{
	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		for (FGameplayTag Tag : AbilitySpec.DynamicAbilityTags)
		{
			if (Tag.MatchesTagExact(InputTag))
			{
				return &AbilitySpec;
			}
		}
	}
	return nullptr;
}

FGameplayAbilitySpec* UD1AbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag& AbilityTag)
{
	ABILITYLIST_SCOPE_LOCK();
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

void UD1AbilitySystemComponent::ClearSlot(const FGameplayTag& SlotTag)
{
	const FD1GameplayTags& Tags = FD1GameplayTags::Get();
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(SlotTag))
		{
			Spec.DynamicAbilityTags.RemoveTag(SlotTag);

			if (GetStatusFromSpec(Spec).MatchesTagExact(Tags.Abilities_Status_Equipped))
			{
				Spec.GetDynamicSpecSourceTags().RemoveTag(Tags.Abilities_Status_Equipped);
				Spec.GetDynamicSpecSourceTags().AddTag(Tags.Abilities_Status_Unlocked);
			}

			MarkAbilitySpecDirty(Spec);
			ClientUpdateAbilityStatus(GetAbilityTagFromSpec(Spec), GetStatusFromSpec(Spec), FGameplayTag(), Spec.Level);
			break;
		}
	}
}

void UD1AbilitySystemComponent::UpdateAbilityStatuses(int32 Level)
{
	if (!GetOwner()->HasAuthority()) return;

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
			// 1. 스펙 생성 (레벨 1)
			FGameplayAbilitySpec NewSpec = FGameplayAbilitySpec(Info.Ability, 1);

			// 2. 초기 상태를 Eligible로 설정
			NewSpec.GetDynamicSpecSourceTags().AddTag(FD1GameplayTags::Get().Abilities_Status_Eligible);

			// 3. 서버 ASC에 등록 (매우 중요!)
			GiveAbility(NewSpec);

			// 4. 클라이언트 UI에 즉시 방송
			ClientUpdateAbilityStatus(Info.AbilityTag, FD1GameplayTags::Get().Abilities_Status_Eligible, FGameplayTag(), NewSpec.Level);

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

				ClientUpdateAbilityStatus(Info.AbilityTag, EligibleTag, FGameplayTag(), AbilitySpec->Level);
			}
		}
	}
}

void UD1AbilitySystemComponent::ServerUpgradeAbility_Implementation(const FGameplayTag& AbilityTag)
{
	FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag);
	if (!AbilitySpec)
	{
		UE_LOG(LogTemp, Error, TEXT("[Upgrade] Spec not found for Tag: %s"), *AbilityTag.ToString());
		return;
	}

	AActor* Avatar = GetAvatarActor();
	if (Avatar && Avatar->Implements<UPlayerInterface>())
	{
		// 1. 포인트 체크
		const int32 CurrentPoints = IPlayerInterface::Execute_GetSkillPoints(Avatar);
		UE_LOG(LogTemp, Log, TEXT("[Upgrade] Start - Ability: %s, Current Points: %d"), *AbilityTag.ToString(), CurrentPoints);

		if (CurrentPoints <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Upgrade] Failed - Not enough skill points."));
			return;
		}

		const FD1GameplayTags& Tags = FD1GameplayTags::Get();
		FGameplayTag StatusTag = GetStatusFromSpec(*AbilitySpec);
		bool bAbilityUpgraded = false;

		UE_LOG(LogTemp, Log, TEXT("[Upgrade] Current Status: %s, Current Level: %d"), *StatusTag.ToString(), AbilitySpec->Level);

		// 2. [배우기] Eligible -> Unlocked
		if (StatusTag.MatchesTagExact(Tags.Abilities_Status_Eligible))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Upgrade] Action: Unlocking Ability (Eligible -> Unlocked)"));
			AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(Tags.Abilities_Status_Eligible);
			AbilitySpec->GetDynamicSpecSourceTags().AddTag(Tags.Abilities_Status_Unlocked);
			bAbilityUpgraded = true;
		}
		// 3. [레벨업] Unlocked/Equipped -> Level Up
		else if (StatusTag.MatchesTagExact(Tags.Abilities_Status_Unlocked) || StatusTag.MatchesTagExact(Tags.Abilities_Status_Equipped))
		{
			if (AbilitySpec->Level < 10)
			{
				AbilitySpec->Level += 1;
				bAbilityUpgraded = true;
				UE_LOG(LogTemp, Warning, TEXT("[Upgrade] Action: Level Up! New Level: %d"), AbilitySpec->Level);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[Upgrade] Failed - Ability is already at Max Level (10)."));
			}
		}
		else
		{
			// 만약 이 로그가 찍힌다면 상태 태그 자체가 로직 외의 것(예: Locked)으로 잡혀있는 겁니다.
			UE_LOG(LogTemp, Error, TEXT("[Upgrade] Failed - StatusTag '%s' is not valid for upgrade."), *StatusTag.ToString());
		}

		// 4. 성공 시 처리
		if (bAbilityUpgraded)
		{
			IPlayerInterface::Execute_AddToSkillPoints(Avatar, -1);
			MarkAbilitySpecDirty(*AbilitySpec);

			FGameplayTag NewStatusTag = GetStatusFromSpec(*AbilitySpec);
			UE_LOG(LogTemp, Log, TEXT("[Upgrade] Success - Remaining Points: %d, Final Status: %s"),
				IPlayerInterface::Execute_GetSkillPoints(Avatar), *NewStatusTag.ToString());

			FGameplayTag CurrentInputTag = GetInputTagFromSpec(*AbilitySpec);
			ClientUpdateAbilityStatus(AbilityTag, NewStatusTag, CurrentInputTag, AbilitySpec->Level);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Upgrade] Avatar is null or doesn't implement UPlayerInterface."));
	}
}

void UD1AbilitySystemComponent::ServerEquipAbility_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& SlotTag)
{
	ClearSlot(SlotTag);

	FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag);
	if (!AbilitySpec) return;

	const FD1GameplayTags& Tags = FD1GameplayTags::Get();

	// 1. [상태 변경] Unlocked -> Equipped (이미 Equipped라면 유지)
	FGameplayTag CurrentStatus = GetStatusFromSpec(*AbilitySpec);
	if (CurrentStatus.MatchesTagExact(Tags.Abilities_Status_Unlocked))
	{
		AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(Tags.Abilities_Status_Unlocked);
		AbilitySpec->GetDynamicSpecSourceTags().AddTag(Tags.Abilities_Status_Equipped);
	}

	// 2. [인풋 변경] 새로운 인풋 태그 부여
	AbilitySpec->DynamicAbilityTags.RemoveTag(GetInputTagFromSpec(*AbilitySpec));
	AbilitySpec->DynamicAbilityTags.AddTag(SlotTag);

	MarkAbilitySpecDirty(*AbilitySpec);

	// 이제 GetStatusFromSpec을 호출하면 방금 넣은 'Equipped' 태그를 제대로 찾아옵니다.
	ClientUpdateAbilityStatus(AbilityTag, GetStatusFromSpec(*AbilitySpec), SlotTag, AbilitySpec->Level);
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

void UD1AbilitySystemComponent::ClientUpdateAbilityStatus_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, const FGameplayTag& InputTag, int32 NewLevel)
{
	UE_LOG(LogTemp, Warning, TEXT("[ClientRPC] Received - Tag: %s, Status: %s, NewLevel: %d"),
		*AbilityTag.ToString(), *StatusTag.ToString(), NewLevel);

	// 자기 Spec 확인 없이 서버가 준 값을 신뢰하고 브로드캐스트
	AbilityStatusChanged.Broadcast(AbilityTag, StatusTag, InputTag, NewLevel);
}

void UD1AbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);

	EffectAssetTags.Broadcast(TagContainer);
}
