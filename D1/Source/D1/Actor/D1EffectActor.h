// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "D1EffectActor.generated.h"

UCLASS()
class D1_API AD1EffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AD1EffectActor();

protected:
	virtual void BeginPlay() override;

private:
	
};
