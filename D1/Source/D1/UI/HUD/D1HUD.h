// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "D1HUD.generated.h"

class UD1UserWidget;
class UD1OverlayWidgetController;
class UD1AttributeMenuWidgetController;
class UAbilitySystemComponent;
class UAttributeSet;
class UD1SkillMenuWidgetController;
class UD1InventoryWidgetController;
class UD1DungeonResultWidgetController;
class UD1DungeonPartyWidgetController;
struct FWidgetControllerParams;

/**
 * 
 */
UCLASS()
class D1_API AD1HUD : public AHUD
{
	GENERATED_BODY()
	
public:

	UD1OverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UD1AttributeMenuWidgetController* GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams);
	UD1SkillMenuWidgetController* GetSkillMenuWidgetController(const FWidgetControllerParams& WCParams);
	UD1InventoryWidgetController* GetInventoryWidgetController(const FWidgetControllerParams& WCParams);
	UD1DungeonResultWidgetController* GetDungeonResultWidgetController(const FWidgetControllerParams& WCParams);
	UD1DungeonPartyWidgetController* GetDungeonPartyWidgetController(const FWidgetControllerParams& WCParams);

	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

private:
	UPROPERTY()
	TObjectPtr<UD1UserWidget> OverlayWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UD1UserWidget> OverlayWidgetClass;

	UPROPERTY()
	TObjectPtr<UD1OverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UD1OverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UD1AttributeMenuWidgetController> AttributeMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UD1AttributeMenuWidgetController> AttributeMenuWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UD1SkillMenuWidgetController> SkillMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UD1SkillMenuWidgetController> SkillMenuWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UD1InventoryWidgetController> InventoryWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UD1InventoryWidgetController> InventoryWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UD1DungeonResultWidgetController> DungeonResultWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UD1DungeonResultWidgetController> DungeonResultWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UD1DungeonPartyWidgetController> DungeonPartyWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UD1DungeonPartyWidgetController> DungeonPartyWidgetControllerClass;
};
