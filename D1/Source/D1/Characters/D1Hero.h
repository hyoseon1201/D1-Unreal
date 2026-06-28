// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/D1CharacterBase.h"
#include "Interaction/PlayerInterface.h"
#include "D1Hero.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UNiagaraComponent;
class UCameraComponent;
class USpringArmComponent;
class AD1PlayerState;

/**
 * 
 */
UCLASS()
class D1_API AD1Hero : public AD1CharacterBase, public IPlayerInterface
{
	GENERATED_BODY()

public:
	AD1Hero();

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	// Begin combat interface
	virtual int32 GetPlayerLevel_Implementation() override;
	// End combat interface

	// Begin player interface
	virtual void AddToXP_Implementation(int32 InXP) override;
	virtual int32 GetXP_Implementation() const override;
	virtual void LevelUp_Implementation() override;
	virtual int32 FindLevelForXP_Implementation(int32 InXP) const override;
	virtual int32 GetAttributePointsReward_Implementation(int32 Level) const override;
	virtual int32 GetSkillPointsReward_Implementation(int32 Level) const override;
	virtual void AddToPlayerLevel_Implementation(int32 InPlayerLevel) override;
	virtual void AddToSkillPoints_Implementation(int32 InSkillPoints) override;
	virtual void AddToAttributePoints_Implementation(int32 InAttributePoints) override;
	virtual int32 GetAttributePoints_Implementation() const override;
	virtual int32 GetSkillPoints_Implementation() const override;
	virtual FGameplayTag GetCharacterClassTag_Implementation() const override;
	// End player interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> LevelUpNiagaraComponent;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	virtual void InitAbilityActorInfo() override;

	/** 플레이어 어빌리티 발동 성공 시 로그 (몬스터 제외, 서버 권한에서만 바인딩) */
	void OnAbilityActivated(UGameplayAbility* Ability);

	/** 플레이어 어빌리티 발동 실패 시 실패 이유 태그까지 로그 (몬스터 제외) */
	void OnAbilityActivationFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);

	// ----- PossessedBy 접속 경로별 GAS 초기화 -----

	/** 최초 스폰 (PIE 등): 직업 ScalableFloat 기본값으로 전체 초기화 */
	void InitializeFreshSpawn(AD1PlayerState* D1PS);

	/** 같은 프로세스 맵 이동: Secondary/Vital 재적용 + GameInstance 저장 데이터 복원 */
	void InitializeFromTravel(AD1PlayerState* D1PS);

	/** [서버 전용] 웹서버 로그인/크로스 프로세스 접속: verify-session으로 DB 데이터 비동기 로드 후 적용 */
	void InitializeFromDbLogin(AD1PlayerState* D1PS);

	/** Primary 확정 후 공통 마무리: Secondary 재계산 + 어빌리티 재등록/상태 갱신/복원 */
	void FinalizeGAS(AD1PlayerState* D1PS);

	/** 체력·마나를 최대치로 충전 (MaxHealth/MaxMana 확정 이후 호출) */
	void RefillVitals();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLevelupParticles() const;
};
