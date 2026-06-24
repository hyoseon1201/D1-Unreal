// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "D1Portal.generated.h"

class UBoxComponent;

/**
 * Town ↔ Dungeon/HuntingGround 등 크로스 프로세스 이동을 트리거하는 포탈 액터.
 * Destination/DestinationMapName을 에디터에서 인스턴스별로 설정해 재사용한다.
 */
UCLASS()
class D1_API AD1Portal : public AActor
{
	GENERATED_BODY()

public:
	AD1Portal();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> PortalCollision;

	/** 백엔드로 전달할 목적지 식별자 ("town" / "dungeon" / "huntingground") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "D1|Portal")
	FString Destination;

	/** 크로스 프로세스가 아닌 단일 프로세스(PIE 등)에서 ServerTravel 폴백용 맵 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "D1|Portal")
	FName DestinationMapName;

	/** 도착 레벨에서 스폰될 PlayerStart의 PlayerStartTag. ClientTravel URL의 "#태그"로 전달됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "D1|Portal")
	FName DestinationPlayerStartTag;
};
