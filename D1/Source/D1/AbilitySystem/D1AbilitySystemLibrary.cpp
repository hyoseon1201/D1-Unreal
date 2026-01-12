// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/D1AbilitySystemLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "UI/HUD/D1HUD.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "Player/D1PlayerState.h"

UD1OverlayWidgetController* UD1AbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AD1HUD* D1HUD = Cast<AD1HUD>(PC->GetHUD()))
		{
			AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();
			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			return D1HUD->GetOverlayWidgetController(WidgetControllerParams);
		}
	}

	return nullptr;
}

UD1AttributeMenuWidgetController* UD1AbilitySystemLibrary::GetAttributeMenuWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AD1HUD* D1HUD = Cast<AD1HUD>(PC->GetHUD()))
		{
			AD1PlayerState* PS = PC->GetPlayerState<AD1PlayerState>();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AS = PS->GetAttributeSet();
			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			return D1HUD->GetAttributeMenuWidgetController(WidgetControllerParams);
		}
	}

	return nullptr;
}
