// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "D1GameStateBase.generated.h"

class UD1ItemRegistry;

/**
 * 클라이언트/서버 모두에서 접근 가능한 게임 상태
 * ItemRegistry 등 공유 데이터를 보관
 */
UCLASS()
class D1_API AD1GameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** 아이템 메타데이터 레지스트리 (서버가 설정, 클라이언트가 읽기) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data")
	TObjectPtr<UD1ItemRegistry> ItemRegistry;
};
