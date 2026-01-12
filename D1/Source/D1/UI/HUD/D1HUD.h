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
};
