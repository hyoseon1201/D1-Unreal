// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "D1HttpSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FD1HttpResponseDelegate, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FD1LoginResponseDelegate, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FD1GetCharactersResponseDelegate, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FD1CreateCharacterResponseDelegate, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FD1EnterTownResponseDelegate, bool, bSuccess, const FString&, ErrorMessage);

USTRUCT(BlueprintType)
struct FD1CharacterInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int64 CharacterId = 0;
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString ClassType;
	UPROPERTY(BlueprintReadOnly) int32 Level = 1;
};

/** verify-session 응답의 stats 부분 (데디서버 전용). bHasStats=false면 신규 캐릭터(DB에 stats 행 없음). */
USTRUCT()
struct FD1LoadedStats
{
	GENERATED_BODY()

	bool  bHasStats       = false;   // stats == null → 신규 캐릭터
	int32 Level           = 1;
	int32 XP              = 0;
	int32 AttributePoints = 0;
	int32 SkillPoints     = 0;
	float Strength        = 0.f;
	float Intelligence    = 0.f;
	float Dexterity       = 0.f;
	float Luck            = 0.f;
};

/** verify-session 응답의 인벤토리 슬롯 1칸 */
USTRUCT()
struct FD1LoadedInventoryItem
{
	GENERATED_BODY()

	int32   SlotIndex = 0;
	FString ItemAssetId;
	int32   Quantity  = 1;
};

/** verify-session 응답의 장착 장비 1개 */
USTRUCT()
struct FD1LoadedEquippedItem
{
	GENERATED_BODY()

	FString SlotType;       // EEquipmentSlot enum 이름 (Weapon/Helmet/...)
	FString ItemAssetId;
};

/** verify-session 응답의 습득 스킬 1개 (Unlocked/Equipped만 저장됨) */
USTRUCT()
struct FD1LoadedSkill
{
	GENERATED_BODY()

	FString SkillTag;       // GameplayTag 문자열
	int32   SkillLevel = 1;
};

/** verify-session 응답의 스킬 장착 슬롯 1개 */
USTRUCT()
struct FD1LoadedSkillSlot
{
	GENERATED_BODY()

	FString SlotKey;        // Q / W / E / R
	FString SkillTag;       // 해당 슬롯에 장착된 스킬 GameplayTag
};

/** verify-session으로 로드한 캐릭터 데이터 전체 (stats + 인벤토리 + 장비 + 스킬; 아이템 퀵슬롯은 게임 미구현으로 제외) */
USTRUCT()
struct FD1LoadedCharacter
{
	GENERATED_BODY()

	FD1LoadedStats Stats;
	TArray<FD1LoadedInventoryItem> Inventory;
	TArray<FD1LoadedEquippedItem>  Equipped;
	TArray<FD1LoadedSkill>         Skills;
	TArray<FD1LoadedSkillSlot>     SkillSlots;
};

// 서버 내부 전용(비-dynamic): verify-session 결과를 요청을 건 그 플레이어에게 라우팅
DECLARE_DELEGATE_ThreeParams(FD1VerifySessionDelegate, bool /*bSuccess*/, int64 /*CharacterId*/, const FD1LoadedCharacter& /*Data*/);

// 서버간 이동용 세션 토큰 발급 결과 (Town↔Dungeon ClientTravel 직전 사용)
DECLARE_DELEGATE_ThreeParams(FD1IssueSessionDelegate, bool /*bSuccess*/, const FString& /*SessionToken*/, const FString& /*ServerAddress*/);

/**
 * 웹서버 HTTP 통신 전담 서브시스템.
 * JWT 토큰, AccountId, 선택된 CharacterId를 씬 전환에도 유지한다.
 */
UCLASS()
class D1_API UD1HttpSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** POST /api/auth/register */
	UFUNCTION(BlueprintCallable, Category = "D1|HTTP")
	void Register(const FString& Email, const FString& Password);

	/** POST /api/auth/login */
	UFUNCTION(BlueprintCallable, Category = "D1|HTTP")
	void Login(const FString& Email, const FString& Password);

	/** GET /api/characters */
	UFUNCTION(BlueprintCallable, Category = "D1|HTTP")
	void GetCharacters();

	/** POST /api/characters */
	UFUNCTION(BlueprintCallable, Category = "D1|HTTP")
	void CreateCharacter(const FString& Name, const FString& ClassType);

	/** POST /api/matchmaking/town → 성공 시 세션 토큰을 붙여 데디서버로 ClientTravel */
	UFUNCTION(BlueprintCallable, Category = "D1|HTTP")
	void EnterTown(int64 CharacterId);

	/**
	 * [데디서버 전용] POST /api/server/sessions/verify — 세션 토큰 검증 + 캐릭터 데이터 로드.
	 * 동시 접속 플레이어마다 결과를 라우팅하기 위해 요청별 콜백(OnComplete)을 받는다.
	 */
	void VerifySession(const FString& SessionToken, FD1VerifySessionDelegate OnComplete);

	/**
	 * [데디서버 전용] POST /api/server/characters/{id}/save — 캐릭터 데이터 전체 일괄 저장.
	 * fire-and-forget: 요청 본문(JsonBody)은 호출 시점에 복사되므로 PS가 곧 소멸해도 POST는 완료된다.
	 */
	void SaveCharacter(int64 CharacterId, const FString& JsonBody);

	/**
	 * [데디서버 전용] POST /api/server/sessions/issue — 서버간 이동용 세션 토큰 발급.
	 * Destination: "town" 또는 "dungeon". 응답으로 SessionToken + ServerAddress 수신 후 ClientTravel에 사용.
	 */
	void IssueSessionToken(int64 CharacterId, const FString& Destination, FD1IssueSessionDelegate OnComplete);

	// 로그인 성공 후 채워지는 세션 데이터
	UPROPERTY(BlueprintReadOnly, Category = "D1|Session")
	FString AuthToken;

	UPROPERTY(BlueprintReadOnly, Category = "D1|Session")
	int64 AccountId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "D1|Session")
	int64 SelectedCharacterId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "D1|Session")
	TArray<FD1CharacterInfo> Characters;

	// 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "D1|HTTP")
	FD1LoginResponseDelegate OnLoginResponse;

	UPROPERTY(BlueprintAssignable, Category = "D1|HTTP")
	FD1GetCharactersResponseDelegate OnGetCharactersResponse;

	UPROPERTY(BlueprintAssignable, Category = "D1|HTTP")
	FD1CreateCharacterResponseDelegate OnCreateCharacterResponse;

	UPROPERTY(BlueprintAssignable, Category = "D1|HTTP")
	FD1HttpResponseDelegate OnRegisterResponse;

	// 성공 시엔 즉시 ClientTravel하므로 실패(에러 표시)용으로만 사용
	UPROPERTY(BlueprintAssignable, Category = "D1|HTTP")
	FD1EnterTownResponseDelegate OnEnterTownResponse;

	/** application.yaml의 server.url 대신 여기서 관리 (로컬 개발용 기본값) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "D1|HTTP")
	FString BaseUrl = TEXT("http://localhost:8080");

	/** [데디서버 전용] 웹서버 서버-API 인증 키. ⚠️ 클라 빌드에도 포함되는 점 유의 (MVP). 운영은 서버 전용 주입으로 분리 예정. */
	UPROPERTY(EditAnywhere, Category = "D1|HTTP")
	FString ServerApiKey = TEXT("d1-server-api-key-change-in-production");

private:
	void OnLoginCompleted(TSharedPtr<class IHttpRequest> Request, TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully);
	void OnRegisterCompleted(TSharedPtr<class IHttpRequest> Request, TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully);
	void OnGetCharactersCompleted(TSharedPtr<class IHttpRequest> Request, TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully);
	void OnCreateCharacterCompleted(TSharedPtr<class IHttpRequest> Request, TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully);
	void OnEnterTownCompleted(TSharedPtr<class IHttpRequest> Request, TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully);

	TSharedRef<class IHttpRequest> MakeRequest(const FString& Verb, const FString& Path);
};
