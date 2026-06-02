// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/D1PlayerState.h"

#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "Inventory/D1InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/D1GameInstance.h"

AD1PlayerState::AD1PlayerState()
{
	UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] PlayerState CONSTRUCTOR called. This=%p"), this);

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
	if (UD1GameInstance* GI = Cast<UD1GameInstance>(GetGameInstance()))
	{
		if (!GI->HasSavedData())
		{
			UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] RestoreTravelDataIfNeeded: No saved data (first login)"));
			return false;
		}

		int32 SavedAttrPts = -1, SavedLvl = -1, SavedXPVal = -1;
		float SavedStr = -1.f, SavedInt = -1.f, SavedDex = -1.f, SavedLuc = -1.f;

		GI->RestorePlayerStateData(
			SavedAttrPts, SavedLvl, SavedXPVal,
			SavedStr, SavedInt, SavedDex, SavedLuc);

		if (SavedAttrPts < 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] RestoreTravelDataIfNeeded: Invalid saved data"));
			return false;
		}

		UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] RestoreTravelDataIfNeeded. BEFORE: bInit=%s, AttrPts=%d, Level=%d, XP=%d"),
			bAbilitySystemInitialized ? TEXT("TRUE") : TEXT("FALSE"),
			AttributePoints, Level, XP);

		// ★ bAbilitySystemInitialized는 복원하지 않음
		// 맵 이동 시 Ability 재등록을 위해 false로 유지

		// 복원 전 현재 값 저장 (변경 여부 비교용)
		const int32 OldAttributePointsBeforeRestore = AttributePoints;
		const int32 OldLevelBeforeRestore = Level;
		const int32 OldXPBeforeRestore = XP;

		// 1. 스칼라 값 복원
		AttributePoints = SavedAttrPts;
		Level = SavedLvl;
		XP = SavedXPVal;

		// 2. Primary Attribute 복원
		if (UD1AttributeSet* AS = Cast<UD1AttributeSet>(AttributeSet))
		{
			AS->SetStrength(SavedStr);
			AS->SetIntelligence(SavedInt);
			AS->SetDexterity(SavedDex);
			AS->SetLuck(SavedLuc);

			UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] Primary Attributes restored. Str=%.1f, Int=%.1f, Dex=%.1f, Luc=%.1f"),
				SavedStr, SavedInt, SavedDex, SavedLuc);
		}

		// 3. 값이 실제로 변경되었을 때만 OnRep 호출 (맵 이동 시 레벨업 이펙트 중복 방지)
		if (AttributePoints != OldAttributePointsBeforeRestore)
		{
			OnRep_AttributePoints(AttributePoints);
		}
		if (Level != OldLevelBeforeRestore)
		{
			OnRep_Level(Level);
		}
		if (XP != OldXPBeforeRestore)
		{
			OnRep_XP(XP);
		}

		UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] RestoreTravelDataIfNeeded. AFTER: bInit=%s, AttrPts=%d, Level=%d, XP=%d"),
			bAbilitySystemInitialized ? TEXT("TRUE") : TEXT("FALSE"),
			AttributePoints, Level, XP);

		// 한 번 복원 후 저장 데이터 초기화
		GI->ClearSavedData();
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TravelDebug] RestoreTravelDataIfNeeded: GameInstance is NOT UD1GameInstance! Class=%s"),
			*GetNameSafe(GetGameInstance()));
		return false;
	}
}

void AD1PlayerState::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[TravelDebug] PlayerState::BeginPlay. This=%p, Authority=%s, bInit=%s, AttrPoints=%d, Level=%d"),
		this,
		HasAuthority() ? TEXT("TRUE") : TEXT("FALSE"),
		bAbilitySystemInitialized ? TEXT("TRUE") : TEXT("FALSE"),
		AttributePoints,
		Level);

	// 맵 이동(ClientTravel) 후 데이터 복원 (PossessedBy보다 늦을 수 있으므로 안전망)
	RestoreTravelDataIfNeeded();

	// 서버에서만 테스트 아이템 지급 (인벤토리 UI 테스트용)
	// 맵 이동(ClientTravel) 후 중복 지급 방지: bAbilitySystemInitialized로 체크
	if (HasAuthority() && InventoryComponent && !bAbilitySystemInitialized)
	{
		InventoryComponent->AddItem(FName("Potion_Health_Small"), 5);
		InventoryComponent->AddItem(FName("Sword_Iron"), 1);
	}
}
