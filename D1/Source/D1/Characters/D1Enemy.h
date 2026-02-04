// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/D1CharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "Interaction/HighlightInterface.h"
#include "UI/WidgetController/D1OverlayWidgetController.h"
#include "AbilitySystem/Data/D1CharacterClassInfo.h"
#include "D1Enemy.generated.h"

class UWidgetComponent;
class UBehaviorTree;
class AD1AIController;

/**
 * 
 */
UCLASS()
class D1_API AD1Enemy : public AD1CharacterBase, public IEnemyInterface, public IHighlightInterface
{
	GENERATED_BODY()
	
public:
	AD1Enemy();

	virtual void PossessedBy(AController* NewController) override;

	/* Combat Interface */
	virtual int32 GetPlayerLevel_Implementation() override;

	virtual void Die() override;

	/* Enemy Interface */
	virtual void SetCombatTarget_Implementation(AActor* InCombatTarget) override;
	virtual AActor* GetCombatTarget_Implementation() const override;

	/* Highlight Interface */
	virtual void HighlightActor_Implementation() override;

	virtual void UnHighlightActor_Implementation() override;

	virtual void SetMoveToLocation_Implementation(FVector& OutDestination) override;

	void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bHitReacting = false;

	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float LifeSpan = 3.f;

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;
	virtual void InitializeDefaultAttributes() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Class Defaults")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Class Defaults")
	ECharacterClass CharacterClass = ECharacterClass::Enemy_Melee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> HealthBar;

	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY()
	TObjectPtr<AD1AIController> D1AIController;
};
