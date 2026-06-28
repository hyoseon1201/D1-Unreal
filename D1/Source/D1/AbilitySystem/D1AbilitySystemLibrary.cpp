// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/D1AbilitySystemLibrary.h"

#include "D1/D1.h"
#include "Kismet/GameplayStatics.h"
#include "UI/HUD/D1HUD.h"
#include "Player/D1PlayerState.h"
#include "Game/D1GameModeBase.h"
#include "Game/D1GameStateBase.h"
#include "Interaction/CombatInterface.h"
#include "D1AbilityTypes.h"
#include "Engine/OverlapResult.h"
#include "AbilitySystem/Data/D1AbilitySystemConfig.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "UI/WidgetController/D1DungeonResultWidgetController.h"
#include "UI/WidgetController/D1DungeonPartyWidgetController.h"
#include "UI/WidgetController/D1GameMenuWidgetController.h"
#include "Inventory/D1ItemRegistry.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "Characters/D1CharacterBase.h"

bool UD1AbilitySystemLibrary::MakeWidgetControllerParams(const UObject* WorldContextObject, FWidgetControllerParams& OutWCParams, AD1HUD*& OutD1HUD)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		OutD1HUD = Cast<AD1HUD>(PC->GetHUD());
		if (OutD1HUD)
		{
			AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();
			OutWCParams.AttributeSet = AS;
			OutWCParams.AbilitySystemComponent = ASC;
			OutWCParams.PlayerState = PS;
			OutWCParams.PlayerController = PC;
			return true;
		}
	}

	return false;
}

UD1OverlayWidgetController* UD1AbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AD1HUD* D1HUD = nullptr;
	const bool bSuccessfulParams = MakeWidgetControllerParams(WorldContextObject, WCParams, D1HUD);

	if (bSuccessfulParams)
	{
		return D1HUD->GetOverlayWidgetController(WCParams);
	}

	return nullptr;
}

UD1AttributeMenuWidgetController* UD1AbilitySystemLibrary::GetAttributeMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AD1HUD* D1HUD = nullptr;
	const bool bSuccessfulParams = MakeWidgetControllerParams(WorldContextObject, WCParams, D1HUD);

	if (bSuccessfulParams)
	{
		return D1HUD->GetAttributeMenuWidgetController(WCParams);
	}

	return nullptr;
}

UD1SkillMenuWidgetController* UD1AbilitySystemLibrary::GetSkillMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AD1HUD* D1HUD = nullptr;
	const bool bSuccessfulParams = MakeWidgetControllerParams(WorldContextObject, WCParams, D1HUD);

	if (bSuccessfulParams)
	{
		return D1HUD->GetSkillMenuWidgetController(WCParams);
	}

	return nullptr;
}

void UD1AbilitySystemLibrary::InitializeDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	AActor* AvatarActor = ASC->GetAvatarActor();

	UD1CharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	FCharacterClassDefaultInfo ClassDefaultInfo = GetCharacterClassInfo(WorldContextObject)->GetClassDefaultInfo(CharacterClass);
	
	FGameplayEffectContextHandle PrimaryAttributesContextHandle = ASC->MakeEffectContext();
	PrimaryAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle PrimaryAttributesSpecHandle = ASC->MakeOutgoingSpec(ClassDefaultInfo.PrimaryAttribute, Level, PrimaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributesSpecHandle.Data.Get());

	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, Level, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	FGameplayEffectContextHandle VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes, Level, VitalAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());
}

void UD1AbilitySystemLibrary::RecalculateSecondaryAttributes(const UObject* WorldContextObject, UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC))
	{
		UE_LOG(LogD1Ability, Error, TEXT("RecalculateSecondaryAttributes: ASC is invalid!"));
		return;
	}

	AActor* AvatarActor = ASC->GetAvatarActor();
	int32 CurrentLevel = 1;
	if (AvatarActor && AvatarActor->Implements<UCombatInterface>())
	{
		CurrentLevel = ICombatInterface::Execute_GetPlayerLevel(AvatarActor);
	}

	// 캐릭터 자신의 DefaultSecondaryAttributes를 우선 사용 (플레이어: Instant GE)
	// 없을 경우 CharacterClassInfo->SecondaryAttributes를 폴백으로 사용 (몬스터용)
	TSubclassOf<UGameplayEffect> SecondaryGEClass = nullptr;
	if (const AD1CharacterBase* CharBase = Cast<AD1CharacterBase>(AvatarActor))
	{
		SecondaryGEClass = CharBase->GetDefaultSecondaryAttributesGE();
	}

	if (!SecondaryGEClass)
	{
		UD1CharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
		if (!CharacterClassInfo || !CharacterClassInfo->SecondaryAttributes)
		{
			UE_LOG(LogD1Ability, Error, TEXT("RecalculateSecondaryAttributes: SecondaryAttributes GE not found on character or CharacterClassInfo!"));
			return;
		}
		SecondaryGEClass = CharacterClassInfo->SecondaryAttributes;
	}

	// 기존 SecondaryAttributes GE를 모두 제거 (중복 적용 방지)
	FGameplayEffectQuery Query;
	Query.EffectDefinition = SecondaryGEClass;
	const int32 RemovedCount = ASC->RemoveActiveEffects(Query);

	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(AvatarActor);
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(SecondaryGEClass, (float)CurrentLevel, SecondaryAttributesContextHandle);

	if (SecondaryAttributesSpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

		const float DBG_AP_Current = ASC->GetNumericAttribute(UD1AttributeSet::GetAttackPowerAttribute());
		const float DBG_AP_Base   = ASC->GetNumericAttributeBase(UD1AttributeSet::GetAttackPowerAttribute());
		UE_LOG(LogD1Ability, Verbose,
			TEXT("RecalculateSecondaryAttributes: Level=%d, removed=%d, AP_CurrentValue=%.0f, AP_BaseValue=%.0f"),
			CurrentLevel, RemovedCount, DBG_AP_Current, DBG_AP_Base);
	}
	else
	{
		UE_LOG(LogD1Ability, Error, TEXT("RecalculateSecondaryAttributes: Failed to create spec!"));
	}
}

void UD1AbilitySystemLibrary::GiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, ECharacterClass CharacterClass)
{
	UD1CharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) return;

	for (TSubclassOf<UGameplayAbility> AbilityClass : CharacterClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}
	const FCharacterClassDefaultInfo DefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultInfo.StartupAbilities)
	{
		AActor* AvatarActor = ASC->GetAvatarActor();

		if (AvatarActor && AvatarActor->Implements<UCombatInterface>())
		{
			const int32 PlayerLevel = ICombatInterface::Execute_GetPlayerLevel(AvatarActor);

			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, PlayerLevel);
			ASC->GiveAbility(AbilitySpec);
		}
	}
}

UD1CharacterClassInfo* UD1AbilitySystemLibrary::GetCharacterClassInfo(const UObject* WorldContextObject)
{
	// 방법 1: WorldContextObject에서 직접 GameMode 찾기 (기존 방식)
	if (const AD1GameModeBase* D1GameMode = Cast<AD1GameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return D1GameMode->CharacterClassInfo;
	}
	
	// 방법 2: WorldContextObject에서 World를 얻어서 GameMode 직접 찾기 (Fallback)
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (const AD1GameModeBase* D1GameMode = Cast<AD1GameModeBase>(World->GetAuthGameMode()))
		{
			return D1GameMode->CharacterClassInfo;
		}
	}
	
	UE_LOG(LogD1Ability, Error, TEXT("GetCharacterClassInfo: Failed to find CharacterClassInfo. Check BP_GameMode settings."));
	return nullptr;
}

UD1AbilityInfo* UD1AbilitySystemLibrary::GetAbilityInfo(const UObject* WorldContextObject)
{
	const AD1GameModeBase* D1GameMode = Cast<AD1GameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (D1GameMode == nullptr) return nullptr;
	return D1GameMode->AbilityInfo;
}

UD1ItemData* UD1AbilitySystemLibrary::GetItemData(const UObject* WorldContextObject, const FName& ItemID)
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	const AD1GameStateBase* D1GS = Cast<AD1GameStateBase>(World->GetGameState());
	if (!D1GS || !D1GS->ItemRegistry)
	{
		UE_LOG(LogD1Inventory, Error, TEXT("GetItemData: GameState or ItemRegistry is NULL!"));
		return nullptr;
	}

	UD1ItemData* ItemData = D1GS->ItemRegistry->FindItemData(ItemID);
	if (!ItemData)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("GetItemData: NOT Found: %s"), *ItemID.ToString());
	}
	return ItemData;
}

UD1InventoryWidgetController* UD1AbilitySystemLibrary::GetInventoryWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AD1HUD* D1HUD = nullptr;
	const bool bSuccessfulParams = MakeWidgetControllerParams(WorldContextObject, WCParams, D1HUD);

	if (bSuccessfulParams)
	{
		return D1HUD->GetInventoryWidgetController(WCParams);
	}

	return nullptr;
}

UD1DungeonResultWidgetController* UD1AbilitySystemLibrary::GetDungeonResultWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AD1HUD* D1HUD = nullptr;
	const bool bSuccessfulParams = MakeWidgetControllerParams(WorldContextObject, WCParams, D1HUD);

	if (bSuccessfulParams)
	{
		return D1HUD->GetDungeonResultWidgetController(WCParams);
	}

	return nullptr;
}

UD1DungeonPartyWidgetController* UD1AbilitySystemLibrary::GetDungeonPartyWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AD1HUD* D1HUD = nullptr;
	const bool bSuccessfulParams = MakeWidgetControllerParams(WorldContextObject, WCParams, D1HUD);

	if (bSuccessfulParams)
	{
		return D1HUD->GetDungeonPartyWidgetController(WCParams);
	}

	return nullptr;
}

UD1GameMenuWidgetController* UD1AbilitySystemLibrary::GetGameMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AD1HUD* D1HUD = nullptr;
	const bool bSuccessfulParams = MakeWidgetControllerParams(WorldContextObject, WCParams, D1HUD);

	if (bSuccessfulParams)
	{
		return D1HUD->GetGameMenuWidgetController(WCParams);
	}

	return nullptr;
}

bool UD1AbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FD1GameplayEffectContext* D1EffectContext = static_cast<const FD1GameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return D1EffectContext->IsCriticalHit();
	}
	return false;
}

void UD1AbilitySystemLibrary::SetIsCriticalHit(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, bool bInIsCriticalHit)
{
	if (FD1GameplayEffectContext* D1EffectContext = static_cast<FD1GameplayEffectContext*>(EffectContextHandle.Get()))
	{
		D1EffectContext->SetIsCriticalHit(bInIsCriticalHit);
	}
}

void UD1AbilitySystemLibrary::GetLivePlayersWithinRadius(const UObject* WorldContextObject, TArray<AActor*>& OutOverlappingActors, const TArray<AActor*>& ActorsToIgnore, float Radius, const FVector& SphereOrigin)
{
	FCollisionQueryParams SphereParams;
	SphereParams.AddIgnoredActors(ActorsToIgnore);

	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, SphereOrigin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(Radius), SphereParams);
		for (FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.GetActor()->Implements<UCombatInterface>() && !ICombatInterface::Execute_IsDead(Overlap.GetActor()))
			{
				OutOverlappingActors.AddUnique(ICombatInterface::Execute_GetAvatar(Overlap.GetActor()));
			}
		}
	}
}

bool UD1AbilitySystemLibrary::IsNotFriend(AActor* FirstActor, AActor* SecondActor)
{
	const bool bBothArePlayers = FirstActor->ActorHasTag(FName("Player")) && SecondActor->ActorHasTag(FName("Player"));
	const bool bBothAreEnemies = FirstActor->ActorHasTag(FName("Enemy")) && SecondActor->ActorHasTag(FName("Enemy"));
	const bool bFriends = bBothArePlayers || bBothAreEnemies;
	return !bFriends;
}

UD1AbilitySystemConfig* UD1AbilitySystemLibrary::GetAbilitySystemConfig(const UObject* WorldContextObject)
{
	AD1GameModeBase* D1GM = Cast<AD1GameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (D1GM)
	{
		return D1GM->AbilitySystemConfig;
	}

	return nullptr;
}

int32 UD1AbilitySystemLibrary::GetXPRewardForClassAndLevel(const UObject* WorldContextObject, ECharacterClass CharacterClass, int32 CharacterLevel)
{
	UD1CharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) return 0;

	const FCharacterClassDefaultInfo& Info = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	const float XPReward = Info.XPReward.GetValueAtLevel(CharacterLevel);

	return static_cast<int32>(XPReward);
}
