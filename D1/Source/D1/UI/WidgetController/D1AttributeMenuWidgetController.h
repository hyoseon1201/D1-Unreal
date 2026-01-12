// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/D1WidgetController.h"
#include "D1AttributeMenuWidgetController.generated.h"

class UD1AttributeInfo;
struct FD1AttributeTagInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttributeTagInfoSignature, const FD1AttributeTagInfo&, Info);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class D1_API UD1AttributeMenuWidgetController : public UD1WidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FAttributeTagInfoSignature AttributeInfoDelegate;

protected:

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UD1AttributeInfo> AttributeInfo;

private:

	void BroadcastAttributeInfo(const FGameplayTag& AttributeTag, const FGameplayAttribute& Attribute) const;
};
