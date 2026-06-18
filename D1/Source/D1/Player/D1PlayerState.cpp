// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/D1PlayerState.h"

#include "D1/D1.h"
#include "AbilitySystem/D1AttributeSet.h"
#include "AbilitySystem/D1AbilitySystemComponent.h"
#include "Inventory/D1InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/D1GameInstance.h"
#include "Game/D1HttpSubsystem.h"
#include "D1GameplayTags.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

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
	OnActualLevelUpDelegate.Broadcast(Level);  // XP 획득 경로에서만 호출됨 (복원·동기화 경로는 SetLevel/OnRep_Level 사용)
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

void AD1PlayerState::ApplyLoadedStats(const FD1LoadedStats& Stats)
{
	// 1. 스칼라 값 적용
	const int32 OldAttrPts = AttributePoints, OldLvl = Level, OldXP = XP, OldSkillPts = SkillPoints;
	AttributePoints = Stats.AttributePoints;
	SkillPoints     = Stats.SkillPoints;
	Level           = Stats.Level;
	XP              = Stats.XP;

	// 2. Primary Attribute 적용 (RestoreTravelDataIfNeeded와 동일 메커니즘 — Base 직접 Set)
	if (UD1AttributeSet* AS = Cast<UD1AttributeSet>(AttributeSet))
	{
		AS->SetStrength(Stats.Strength);
		AS->SetIntelligence(Stats.Intelligence);
		AS->SetDexterity(Stats.Dexterity);
		AS->SetLuck(Stats.Luck);
	}

	// 3. 변경된 값만 OnRep 호출 (클라 동기화)
	if (AttributePoints != OldAttrPts) OnRep_AttributePoints(AttributePoints);
	if (SkillPoints     != OldSkillPts) OnRep_SkillPoints(SkillPoints);
	if (Level           != OldLvl)     OnRep_Level(Level);
	if (XP              != OldXP)      OnRep_XP(XP);

	UE_LOG(LogD1Travel, Log, TEXT("ApplyLoadedStats: Lv=%d XP=%d AttrPts=%d SkillPts=%d STR=%.0f"),
		Level, XP, AttributePoints, SkillPoints, Stats.Strength);
}

void AD1PlayerState::ApplyLoadedInventory(const TArray<FD1LoadedInventoryItem>& InInventory,
	const TArray<FD1LoadedEquippedItem>& InEquipped)
{
	if (!InventoryComponent)
	{
		UE_LOG(LogD1Inventory, Warning, TEXT("ApplyLoadedInventory: InventoryComponent 없음"));
		return;
	}

	// DB 타입 → 게임 타입 변환
	TArray<FD1InventoryItem> InventorySlots;
	InventorySlots.Reserve(InInventory.Num());
	for (const FD1LoadedInventoryItem& Src : InInventory)
	{
		FD1InventoryItem Item;
		Item.ItemID    = FName(*Src.ItemAssetId);
		Item.Count     = Src.Quantity;
		Item.SlotIndex = Src.SlotIndex;
		InventorySlots.Add(Item);
	}

	const UEnum* SlotEnum = StaticEnum<EEquipmentSlot>();
	TArray<FD1EquippedItem> EquippedItems;
	EquippedItems.Reserve(InEquipped.Num());
	for (const FD1LoadedEquippedItem& Src : InEquipped)
	{
		// slot_type 문자열 → EEquipmentSlot (qualified/short 둘 다 시도)
		int64 EnumVal = SlotEnum ? SlotEnum->GetValueByNameString(FString(TEXT("EEquipmentSlot::")) + Src.SlotType) : INDEX_NONE;
		if (EnumVal == INDEX_NONE && SlotEnum) EnumVal = SlotEnum->GetValueByNameString(Src.SlotType);
		if (EnumVal == INDEX_NONE || EnumVal == (int64)EEquipmentSlot::None)
		{
			UE_LOG(LogD1Inventory, Warning, TEXT("ApplyLoadedInventory: 알 수 없는 slot_type '%s' — 건너뜀"), *Src.SlotType);
			continue;
		}

		FD1EquippedItem Equip;
		Equip.EquipmentSlot = (EEquipmentSlot)EnumVal;
		Equip.Item.ItemID   = FName(*Src.ItemAssetId);
		Equip.Item.Count    = 1;
		EquippedItems.Add(Equip);
	}

	// 기존 복원 경로 재사용 (장비 GE 재적용 포함)
	InventoryComponent->RestoreFromSave(InventorySlots, EquippedItems);

	UE_LOG(LogD1Inventory, Log, TEXT("ApplyLoadedInventory: 인벤토리 %d칸, 장비 %d개 적용"),
		InventorySlots.Num(), EquippedItems.Num());
}

void AD1PlayerState::ApplyLoadedSkills(const TArray<FD1LoadedSkill>& InSkills,
	const TArray<FD1LoadedSkillSlot>& InSkillSlots)
{
	UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(GetAbilitySystemComponent());
	if (!D1ASC)
	{
		UE_LOG(LogD1Ability, Warning, TEXT("ApplyLoadedSkills: ASC 없음"));
		return;
	}

	const FD1GameplayTags& GameTags = FD1GameplayTags::Get();

	// 슬롯 키(Q/W/E/R) → InputTag
	auto SlotKeyToInputTag = [&GameTags](const FString& Key) -> FGameplayTag
	{
		if (Key == TEXT("Q")) return GameTags.InputTag_Q;
		if (Key == TEXT("W")) return GameTags.InputTag_W;
		if (Key == TEXT("E")) return GameTags.InputTag_E;
		if (Key == TEXT("R")) return GameTags.InputTag_R;
		return FGameplayTag();
	};

	// 어느 스킬이 어느 슬롯에 장착됐는지 (skill_tag → InputTag)
	TMap<FString, FGameplayTag> EquippedSlotByTag;
	for (const FD1LoadedSkillSlot& Slot : InSkillSlots)
	{
		const FGameplayTag InputTag = SlotKeyToInputTag(Slot.SlotKey);
		if (InputTag.IsValid())
		{
			EquippedSlotByTag.Add(Slot.SkillTag, InputTag);
		}
	}

	// DB 스킬 → FD1SavedAbilityInfo 변환
	TArray<FD1SavedAbilityInfo> Saved;
	for (const FD1LoadedSkill& Skill : InSkills)
	{
		FD1SavedAbilityInfo Info;
		Info.AbilityTag = FGameplayTag::RequestGameplayTag(FName(*Skill.SkillTag), /*ErrorIfNotFound*/ false);
		if (!Info.AbilityTag.IsValid())
		{
			UE_LOG(LogD1Ability, Warning, TEXT("ApplyLoadedSkills: 알 수 없는 skill_tag '%s' — 건너뜀"), *Skill.SkillTag);
			continue;
		}
		Info.Level = Skill.SkillLevel;

		// 슬롯에 있으면 Equipped + SlotTag, 없으면 Unlocked
		if (const FGameplayTag* SlotTag = EquippedSlotByTag.Find(Skill.SkillTag))
		{
			Info.StatusTag = GameTags.Abilities_Status_Equipped;
			Info.SlotTag   = *SlotTag;
		}
		else
		{
			Info.StatusTag = GameTags.Abilities_Status_Unlocked;
		}
		Saved.Add(Info);
	}

	D1ASC->RestoreAbilityStates(Saved);
	UE_LOG(LogD1Ability, Log, TEXT("ApplyLoadedSkills: 스킬 %d개 적용 (슬롯 %d개)"), Saved.Num(), InSkillSlots.Num());
}

FString AD1PlayerState::BuildSaveJson()
{
	const UD1AttributeSet* AS = Cast<UD1AttributeSet>(AttributeSet);
	const FD1GameplayTags& GameTags = FD1GameplayTags::Get();

	// InputTag → 슬롯 키(Q/W/E/R) 역변환
	auto InputTagToSlotKey = [&GameTags](const FGameplayTag& Tag) -> FString
	{
		if (Tag == GameTags.InputTag_Q) return TEXT("Q");
		if (Tag == GameTags.InputTag_W) return TEXT("W");
		if (Tag == GameTags.InputTag_E) return TEXT("E");
		if (Tag == GameTags.InputTag_R) return TEXT("R");
		return FString();
	};

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	// stats
	TSharedPtr<FJsonObject> Stats = MakeShared<FJsonObject>();
	Stats->SetNumberField(TEXT("level"),           Level);
	Stats->SetNumberField(TEXT("xp"),              XP);
	Stats->SetNumberField(TEXT("attributePoints"), AttributePoints);
	Stats->SetNumberField(TEXT("skillPoints"),     SkillPoints);
	Stats->SetNumberField(TEXT("strength"),        AS ? AS->GetStrength()     : 0.f);
	Stats->SetNumberField(TEXT("intelligence"),    AS ? AS->GetIntelligence() : 0.f);
	Stats->SetNumberField(TEXT("dexterity"),       AS ? AS->GetDexterity()    : 0.f);
	Stats->SetNumberField(TEXT("luck"),            AS ? AS->GetLuck()         : 0.f);
	Root->SetObjectField(TEXT("stats"), Stats);

	// skills + skillSlots (SaveAbilityStates 기반)
	TArray<TSharedPtr<FJsonValue>> SkillsArr;
	TArray<TSharedPtr<FJsonValue>> SkillSlotsArr;
	if (UD1AbilitySystemComponent* D1ASC = Cast<UD1AbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		for (const FD1SavedAbilityInfo& Info : D1ASC->SaveAbilityStates())
		{
			const FString SkillTag = Info.AbilityTag.ToString();

			TSharedPtr<FJsonObject> SkillObj = MakeShared<FJsonObject>();
			SkillObj->SetStringField(TEXT("skillTag"), SkillTag);
			SkillObj->SetNumberField(TEXT("skillLevel"), Info.Level);
			SkillsArr.Add(MakeShared<FJsonValueObject>(SkillObj));

			// Equipped면 슬롯도 기록
			if (Info.StatusTag.MatchesTagExact(GameTags.Abilities_Status_Equipped) && Info.SlotTag.IsValid())
			{
				const FString SlotKey = InputTagToSlotKey(Info.SlotTag);
				if (!SlotKey.IsEmpty())
				{
					TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
					SlotObj->SetStringField(TEXT("slotKey"), SlotKey);
					SlotObj->SetStringField(TEXT("skillTag"), SkillTag);
					SkillSlotsArr.Add(MakeShared<FJsonValueObject>(SlotObj));
				}
			}
		}
	}
	Root->SetArrayField(TEXT("skills"), SkillsArr);
	Root->SetArrayField(TEXT("skillSlots"), SkillSlotsArr);

	// inventory + equippedItems
	TArray<TSharedPtr<FJsonValue>> InvArr;
	TArray<TSharedPtr<FJsonValue>> EquipArr;
	if (InventoryComponent)
	{
		for (const FD1InventoryItem& Item : InventoryComponent->GetInventorySlots())
		{
			if (Item.ItemID.IsNone() || Item.Count <= 0) continue;
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetNumberField(TEXT("slotIndex"),   Item.SlotIndex);
			Obj->SetStringField(TEXT("itemAssetId"), Item.ItemID.ToString());
			Obj->SetNumberField(TEXT("quantity"),    Item.Count);
			InvArr.Add(MakeShared<FJsonValueObject>(Obj));
		}

		const UEnum* SlotEnum = StaticEnum<EEquipmentSlot>();
		for (const FD1EquippedItem& Equip : InventoryComponent->GetEquippedItems())
		{
			if (Equip.EquipmentSlot == EEquipmentSlot::None || Equip.Item.ItemID.IsNone()) continue;

			// EEquipmentSlot → 짧은 이름 ("EEquipmentSlot::Weapon" → "Weapon")
			FString SlotName = SlotEnum ? SlotEnum->GetNameStringByValue((int64)Equip.EquipmentSlot) : FString();
			int32 ColonIdx;
			if (SlotName.FindLastChar(TEXT(':'), ColonIdx)) SlotName = SlotName.RightChop(ColonIdx + 1);

			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("slotType"),    SlotName);
			Obj->SetStringField(TEXT("itemAssetId"), Equip.Item.ItemID.ToString());
			EquipArr.Add(MakeShared<FJsonValueObject>(Obj));
		}
	}
	Root->SetArrayField(TEXT("inventory"), InvArr);
	Root->SetArrayField(TEXT("equippedItems"), EquipArr);

	// quickSlots: 게임 미구현 → 빈 배열 (백엔드 @NotNull 충족)
	Root->SetArrayField(TEXT("quickSlots"), TArray<TSharedPtr<FJsonValue>>());

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Out;
}

void AD1PlayerState::BeginPlay()
{
	Super::BeginPlay();

	// 맵 이동(Travel) 데이터 복원은 PossessedBy에서 처리
	// (BeginPlay는 PossessedBy보다 먼저 호출되므로 여기서 복원하면 GameInstance 데이터가 소진됨)
	//
	// ※ 기존 테스트 아이템 하드코딩 지급(Potion/Sword)은 제거됨 (Phase 3) —
	//   인벤토리는 웹서버 DB에서 verify-session으로 로드해 ApplyLoadedInventory로 적용.
}
