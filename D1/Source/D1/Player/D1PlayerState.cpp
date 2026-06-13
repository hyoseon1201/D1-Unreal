// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/D1PlayerState.h"

#include "D1/D1.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "Inventory/D1InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/D1GameInstance.h"

AD1PlayerState::AD1PlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UD1AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UD1AttributeSet>("AttributeSet");

	InventoryComponent = CreateDefaultSubobject<UD1InventoryComponent>("InventoryComponent");

	SetNetUpdateFrequency(100.f);
}

void AD1PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AD1PlayerState, Level);
	DOREPLIFETIME(AD1PlayerState, XP);
	DOREPLIFETIME(AD1PlayerState, AttributePoints);
	DOREPLIFETIME(AD1PlayerState, SkillPoints);
	DOREPLIFETIME(AD1PlayerState, InventoryComponent);
	DOREPLIFETIME(AD1PlayerState, bAbilitySystemInitialized);
	DOREPLIFETIME(AD1PlayerState, bAbilitiesAllowed);
}

UAbilitySystemComponent* AD1PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FString AD1PlayerState::GetPartyPlayerId() const
{
	// UniqueNetId가 유효하면 그것을 사용 (세션 내 고유 보장)
	const FUniqueNetIdRepl& NetId = GetUniqueId();
	if (NetId.IsValid())
	{
		return NetId->ToString();
	}
	// OSS 없는 환경(Standalone PIE 등) 폴백
	return GetPlayerName();
}

void AD1PlayerState::AddToXP(int32 InXP)
{
	XP += InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void AD1PlayerState::AddToLevel(int32 InLevel)
{
	Level += InLevel;
	OnLevelChangedDelegate.Broadcast(Level);
}

void AD1PlayerState::AddToAttributePoints(int32 InPoints)
{
	AttributePoints += InPoints;
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AD1PlayerState::AddToSkillPoints(int32 InPoints)
{
	SkillPoints += InPoints;
	OnSkillPointsChangedDelegate.Broadcast(SkillPoints);
}

void AD1PlayerState::SetXP(int32 InXP)
{
	XP = InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void AD1PlayerState::SetLevel(int32 InLevel)
{
	Level = InLevel;
	OnLevelChangedDelegate.Broadcast(Level);
}

void AD1PlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChangedDelegate.Broadcast(Level);
}

void AD1PlayerState::OnRep_XP(int32 OldXP)
{
	OnXPChangedDelegate.Broadcast(XP);
}

void AD1PlayerState::OnRep_AttributePoints(int32 OldAttributePoints)
{
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void AD1PlayerState::OnRep_SkillPoints(int32 OldSkillPoints)
{
	OnSkillPointsChangedDelegate.Broadcast(SkillPoints);
}

bool AD1PlayerState::RestoreTravelDataIfNeeded()
{
	UD1GameInstance* GI = Cast<UD1GameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogD1Travel, Error, TEXT("RestoreTravelDataIfNeeded: GameInstance is NOT UD1GameInstance! Class=%s"),
			*GetNameSafe(GetGameInstance()));
		return false;
	}

	const FString PartyId = GetPartyPlayerId();
	if (!GI->HasSavedData(PartyId))
	{
		UE_LOG(LogD1Travel, Warning, TEXT("RestoreTravelDataIfNeeded [%s]: No saved data — skipping restore"), *PartyId);
		return false;
	}

	FD1SavedPlayerData Data;
	if (!GI->TryGetPlayerData(PartyId, Data))
	{
		UE_LOG(LogD1Travel, Warning, TEXT("RestoreTravelDataIfNeeded [%s]: TryGetPlayerData failed"), *PartyId);
		return false;
	}

	// 1. 스칼라 값 복원
	const int32 OldAttrPts = AttributePoints, OldLvl = Level, OldXP = XP;
	const int32 OldSkillPts = SkillPoints;
	AttributePoints = Data.AttributePoints;
	SkillPoints     = Data.SkillPoints;
	Level           = Data.Level;
	XP              = Data.XP;

	// 2. Primary Attribute 복원
	if (UD1AttributeSet* AS = Cast<UD1AttributeSet>(AttributeSet))
	{
		AS->SetStrength(Data.Strength);
		AS->SetIntelligence(Data.Intelligence);
		AS->SetDexterity(Data.Dexterity);
		AS->SetLuck(Data.Luck);
	}

	// 3. 인벤토리 복원
	if (InventoryComponent)
	{
		InventoryComponent->RestoreFromSave(Data.InventorySlots, Data.EquippedItems);
	}

	// 4. 어빌리티 상태는 PossessedBy의 UpdateAbilityStatuses 완료 후에 적용 (임시 보관)
	PendingAbilityRestoreData = Data.AbilityStates;

	// 5. 변경된 값만 OnRep 호출
	if (AttributePoints != OldAttrPts) OnRep_AttributePoints(AttributePoints);
	if (SkillPoints     != OldSkillPts) OnRep_SkillPoints(SkillPoints);
	if (Level           != OldLvl)     OnRep_Level(Level);
	if (XP              != OldXP)      OnRep_XP(XP);

	UE_LOG(LogD1Travel, Warning, TEXT("RestoreTravelDataIfNeeded [%s]: RESTORED AttrPts=%d, Level=%d, Str=%.1f, Inventory=%d, Equipped=%d, Abilities=%d"),
		*PartyId, AttributePoints, Level, Data.Strength, Data.InventorySlots.Num(), Data.EquippedItems.Num(), Data.AbilityStates.Num());

	GI->ClearPlayerData(PartyId);
	return true;
}

void AD1PlayerState::BeginPlay()
{
	Super::BeginPlay();

	// 맵 이동(Travel) 데이터 복원은 PossessedBy에서 처리
	// (BeginPlay는 PossessedBy보다 먼저 호출되므로 여기서 복원하면 GameInstance 데이터가 소진됨)

	// 서버에서만 최초 입장 시 테스트 아이템 지급
	// 조건: 어빌리티 시스템 미초기화 상태 + Travel 복원 데이터 없음
	UD1GameInstance* GI = Cast<UD1GameInstance>(GetGameInstance());
	const bool bHasTravelData = GI && GI->HasSavedData(GetPartyPlayerId());
	if (HasAuthority() && InventoryComponent && !bAbilitySystemInitialized && !bHasTravelData)
	{
		InventoryComponent->AddItem(FName("Potion_Health_Small"), 5);
		InventoryComponent->AddItem(FName("Sword_Iron"), 1);
	}
}
