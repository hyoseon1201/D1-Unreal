// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "D1GameInstance.generated.h"

/**
 * 맵 이동(ClientTravel) 시 PlayerState 데이터를 임시 저장/복원하는 GameInstance.
 * PIE 및 SeamlessTravel 미지원 환경에서도 데이터 유지를 보장.
 */
UCLASS()
class D1_API UD1GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 맵 이동 전 현재 PlayerState의 영속 데이터를 저장 (Ability 초기화 여부 제외) */
	UFUNCTION(BlueprintCallable, Category = "D1|Travel")
	void SavePlayerStateData(
		int32 InAttributePoints, int32 InLevel, int32 InXP,
		float InStrength, float InIntelligence,
		float InDexterity, float InLuck);

	/** 맵 이동 후 새 PlayerState에 데이터를 복원 (없으면 기본값 사용) */
	UFUNCTION(BlueprintCallable, Category = "D1|Travel")
	void RestorePlayerStateData(
		int32& OutAttributePoints, int32& OutLevel, int32& OutXP,
		float& OutStrength, float& OutIntelligence,
		float& OutDexterity, float& OutLuck) const;

	/** 저장된 데이터를 초기화 (선택: 로그아웃 시) */
	UFUNCTION(BlueprintCallable, Category = "D1|Travel")
	void ClearSavedData();

	/** 저장된 데이터가 있는지 여부 */
	UFUNCTION(BlueprintPure, Category = "D1|Travel")
	bool HasSavedData() const { return bHasSavedData; }

private:
	UPROPERTY()
	int32 SavedAttributePoints = -1;

	UPROPERTY()
	int32 SavedLevel = -1;

	UPROPERTY()
	int32 SavedXP = -1;

	UPROPERTY()
	float SavedStrength = -1.f;

	UPROPERTY()
	float SavedIntelligence = -1.f;

	UPROPERTY()
	float SavedDexterity = -1.f;

	UPROPERTY()
	float SavedLuck = -1.f;

	UPROPERTY()
	bool bHasSavedData = false;
};
