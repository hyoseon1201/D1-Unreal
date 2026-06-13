// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/D1AbilitySystemComponent.h"

#include "D1/D1.h"
#include "D1GameplayTags.h"
#include "AbilitySystem/Ability/D1GameplayAbility.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystem/D1AbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Data/D1AbilityInfo.h"
#include "Inventory/D1InventoryComponent.h"
#include "AbilitySystem/D1AttributeSet.h"

void UD1AbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UD1AbilitySystemComponent::ClientEffectApplied);
}

void UD1AbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
	{
		// 이미 동일한 Ability Class가 부여되어 있으면 스킵 (중복 초기화 방지)
		if (FindAbilitySpecFromClass(AbilityClass) != nullptr)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		if (const UD1GameplayAbility* D1Ability = Cast<UD1GameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(D1Ability->StartupInputTag);
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(FD1GameplayTags::Get().Abilities_Status_Locked);
			GiveAbility(AbilitySpec);
		}
	}

	bStartupAbilitiesGiven = true;

	AbilitiesGivenDelegate.Broadcast();
}

void UD1AbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
	{
		// 이미 동일한 Ability Class가 부여되어 있으면 스킵
		if (FindAbilitySpecFromClass(AbilityClass) != nullptr)
		{
			continue;
		}

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
			UE_LOG(LogD1Ability, Error, TEXT("Failed to execute delegate in %hs"), __FUNCTION__);
		}
	}
}

FGameplayTag UD1AbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability)
	{
		for (FGameplayTag Tag : AbilitySpec.Ability.Get()->GetAssetTags())
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities"))))
			{
				return Tag;
			}
		}
	}

	UE_LOG(LogD1Ability, Error, TEXT("GetAbilityTagFromSpec: Failed to find matching tag for %s!"), *GetNameSafe(AbilitySpec.Ability.Get()));
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
		const int32 Points = IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor());
		if (Points > 0)
		{
			ServerUpgradeAttribute(AttributeTag);
		}
	}
	else
	{
		UE_LOG(LogD1Ability, Error, TEXT("UpgradeAttribute: AvatarActor does not implement UPlayerInterface!"));
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
		UE_LOG(LogD1Ability, Error, TEXT("UpdateAbilityStatuses: AbilityInfo is NULL!"));
		return;
	}

	FGameplayTag MyClassTag;
	if (IPlayerInterface* PlayerInt = Cast<IPlayerInterface>(GetAvatarActor()))
	{
		MyClassTag = PlayerInt->Execute_GetCharacterClassTag(GetAvatarActor());
	}
	else
	{
		UE_LOG(LogD1Ability, Warning, TEXT("UpdateAbilityStatuses: PlayerInterface NOT FOUND on Avatar!"));
	}

	UE_LOG(LogD1Ability, Verbose, TEXT("UpdateAbilityStatuses: Checking abilities for Level [%d], ClassTag [%s]"), Level, *MyClassTag.ToString());

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

			UE_LOG(LogD1Ability, Verbose, TEXT("UpdateAbilityStatuses: giving new ability [%s]"), *Info.AbilityTag.ToString());
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

				UE_LOG(LogD1Ability, Verbose, TEXT("UpdateAbilityStatuses: [%s] Locked -> Eligible"), *Info.AbilityTag.ToString());

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
		UE_LOG(LogD1Ability, Error, TEXT("ServerUpgradeAbility: Spec not found for Tag: %s"), *AbilityTag.ToString());
		return;
	}

	AActor* Avatar = GetAvatarActor();
	if (Avatar && Avatar->Implements<UPlayerInterface>())
	{
		// 1. 포인트 체크
		const int32 CurrentPoints = IPlayerInterface::Execute_GetSkillPoints(Avatar);
		if (CurrentPoints <= 0)
		{
			UE_LOG(LogD1Ability, Warning, TEXT("ServerUpgradeAbility: Not enough skill points for %s"), *AbilityTag.ToString());
			return;
		}

		const FD1GameplayTags& Tags = FD1GameplayTags::Get();
		FGameplayTag StatusTag = GetStatusFromSpec(*AbilitySpec);
		bool bAbilityUpgraded = false;

		// 2. [배우기] Eligible -> Unlocked
		if (StatusTag.MatchesTagExact(Tags.Abilities_Status_Eligible))
		{
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
			}
			else
			{
				UE_LOG(LogD1Ability, Warning, TEXT("ServerUpgradeAbility: %s is already at Max Level (10)."), *AbilityTag.ToString());
			}
		}
		else
		{
			// 이 로그가 찍힌다면 상태 태그 자체가 로직 외의 것(예: Locked)으로 잡혀있는 것
			UE_LOG(LogD1Ability, Error, TEXT("ServerUpgradeAbility: StatusTag '%s' is not valid for upgrade."), *StatusTag.ToString());
		}

		// 4. 성공 시 처리
		if (bAbilityUpgraded)
		{
			IPlayerInterface::Execute_AddToSkillPoints(Avatar, -1);
			MarkAbilitySpecDirty(*AbilitySpec);

			FGameplayTag NewStatusTag = GetStatusFromSpec(*AbilitySpec);
			UE_LOG(LogD1Ability, Verbose, TEXT("ServerUpgradeAbility: %s upgraded. Level=%d, Status=%s"),
				*AbilityTag.ToString(), AbilitySpec->Level, *NewStatusTag.ToString());

			FGameplayTag CurrentInputTag = GetInputTagFromSpec(*AbilitySpec);
			ClientUpdateAbilityStatus(AbilityTag, NewStatusTag, CurrentInputTag, AbilitySpec->Level);
		}
	}
	else
	{
		UE_LOG(LogD1Ability, Error, TEXT("ServerUpgradeAbility: Avatar is null or doesn't implement UPlayerInterface."));
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

TArray<FD1SavedAbilityInfo> UD1AbilitySystemComponent::SaveAbilityStates()
{
	TArray<FD1SavedAbilityInfo> Result;

	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const FGameplayTag StatusTag = GetStatusFromSpec(Spec);

		// Locked 상태는 저장 불필요 — AddCharacterAbilities + UpdateAbilityStatuses가 재생성
		if (!StatusTag.IsValid() || StatusTag.MatchesTagExact(FD1GameplayTags::Get().Abilities_Status_Locked))
		{
			continue;
		}

		const FGameplayTag AbilityTag = GetAbilityTagFromSpec(Spec);
		if (!AbilityTag.IsValid())
		{
			continue;
		}

		FD1SavedAbilityInfo Info;
		Info.AbilityTag = AbilityTag;
		Info.StatusTag  = StatusTag;
		Info.SlotTag    = GetInputTagFromSpec(Spec);  // Equipped 아닐 경우 Invalid
		Info.Level      = Spec.Level;
		Result.Add(Info);
	}

	UE_LOG(LogD1Ability, Warning, TEXT("SaveAbilityStates: saved %d ability states"), Result.Num());
	return Result;
}

void UD1AbilitySystemComponent::RestoreAbilityStates(const TArray<FD1SavedAbilityInfo>& SavedAbilities)
{
	if (!GetOwner()->HasAuthority() || SavedAbilities.IsEmpty())
	{
		return;
	}

	const FD1GameplayTags& Tags = FD1GameplayTags::Get();
	ABILITYLIST_SCOPE_LOCK();

	for (const FD1SavedAbilityInfo& Saved : SavedAbilities)
	{
		if (!Saved.AbilityTag.IsValid()) continue;

		FGameplayAbilitySpec* Spec = GetSpecFromAbilityTag(Saved.AbilityTag);
		if (!Spec)
		{
			UE_LOG(LogD1Ability, Warning, TEXT("RestoreAbilityStates: Spec not found for %s"), *Saved.AbilityTag.ToString());
			continue;
		}

		// 1. 어빌리티 레벨 복원
		Spec->Level = Saved.Level;

		// 2. 상태 태그 교체 (현재 Eligible → 저장된 Unlocked/Equipped)
		const FGameplayTag CurrentStatus = GetStatusFromSpec(*Spec);
		if (CurrentStatus.IsValid())
		{
			Spec->GetDynamicSpecSourceTags().RemoveTag(CurrentStatus);
		}
		Spec->GetDynamicSpecSourceTags().AddTag(Saved.StatusTag);

		// 3. 퀵슬롯 복원 (Equipped 상태일 때만)
		if (Saved.StatusTag.MatchesTagExact(Tags.Abilities_Status_Equipped) && Saved.SlotTag.IsValid())
		{
			const FGameplayTag CurrentSlot = GetInputTagFromSpec(*Spec);
			if (CurrentSlot.IsValid())
			{
				Spec->GetDynamicSpecSourceTags().RemoveTag(CurrentSlot);
			}
			Spec->GetDynamicSpecSourceTags().AddTag(Saved.SlotTag);
		}

		MarkAbilitySpecDirty(*Spec);

		const FGameplayTag RestoredSlot = GetInputTagFromSpec(*Spec);
		ClientUpdateAbilityStatus(Saved.AbilityTag, Saved.StatusTag, RestoredSlot, Spec->Level);

		UE_LOG(LogD1Ability, Log, TEXT("RestoreAbilityStates: %s → Status=%s, Level=%d, Slot=%s"),
			*Saved.AbilityTag.ToString(), *Saved.StatusTag.ToString(), Spec->Level, *RestoredSlot.ToString());
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
	// 자기 Spec 확인 없이 서버가 준 값을 신뢰하고 브로드캐스트
	AbilityStatusChanged.Broadcast(AbilityTag, StatusTag, InputTag, NewLevel);
}

void UD1AbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);

	EffectAssetTags.Broadcast(TagContainer);
}
