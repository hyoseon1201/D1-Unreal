// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

#define CUSTOM_DEPTH_RED 250
#define ECC_Projectile ECollisionChannel::ECC_GameTraceChannel1

/*
 * 프로젝트 전용 로그 카테고리
 * 사용 예: UE_LOG(LogD1Party, Log, TEXT("..."))
 * 콘솔에서 영역별 필터링 가능: Log LogD1Party Verbose
 */
DECLARE_LOG_CATEGORY_EXTERN(LogD1, Log, All);          // 일반
DECLARE_LOG_CATEGORY_EXTERN(LogD1Ability, Log, All);   // GAS / 어빌리티 / 스킬
DECLARE_LOG_CATEGORY_EXTERN(LogD1Party, Log, All);     // 파티 시스템
DECLARE_LOG_CATEGORY_EXTERN(LogD1Inventory, Log, All); // 인벤토리 / 장비
DECLARE_LOG_CATEGORY_EXTERN(LogD1Travel, Log, All);    // 맵 이동 / 데이터 저장·복원
DECLARE_LOG_CATEGORY_EXTERN(LogD1UI, Log, All);        // 위젯 / HUD
