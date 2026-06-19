// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/D1HttpSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "D1.h"

TSharedRef<IHttpRequest> UD1HttpSubsystem::MakeRequest(const FString& Verb, const FString& Path)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + Path);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!AuthToken.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + AuthToken);
	}
	return Request;
}

void UD1HttpSubsystem::Register(const FString& Email, const FString& Password)
{
	TSharedRef<IHttpRequest> Request = MakeRequest(TEXT("POST"), TEXT("/api/auth/register"));

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("email"), Email);
	Body->SetStringField(TEXT("password"), Password);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(this, &UD1HttpSubsystem::OnRegisterCompleted);
	Request->ProcessRequest();
}

void UD1HttpSubsystem::OnRegisterCompleted(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnRegisterResponse.Broadcast(false, TEXT("서버에 연결할 수 없습니다."));
		return;
	}

	if (Response->GetResponseCode() == 201)
	{
		OnRegisterResponse.Broadcast(true, TEXT(""));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FString ErrorMsg = FString::Printf(TEXT("오류 코드: %d"), Response->GetResponseCode());
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		FString Msg;
		if (JsonObject->TryGetStringField(TEXT("message"), Msg))
		{
			ErrorMsg = Msg;
		}
	}
	OnRegisterResponse.Broadcast(false, ErrorMsg);
}

void UD1HttpSubsystem::Login(const FString& Email, const FString& Password)
{
	TSharedRef<IHttpRequest> Request = MakeRequest(TEXT("POST"), TEXT("/api/auth/login"));

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("email"), Email);
	Body->SetStringField(TEXT("password"), Password);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(this, &UD1HttpSubsystem::OnLoginCompleted);
	Request->ProcessRequest();
}

void UD1HttpSubsystem::OnLoginCompleted(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogD1, Warning, TEXT("Login: 서버 연결 실패"));
		OnLoginResponse.Broadcast(false, TEXT("서버에 연결할 수 없습니다."));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		UE_LOG(LogD1, Warning, TEXT("Login: JSON 파싱 실패"));
		OnLoginResponse.Broadcast(false, TEXT("응답 파싱 실패"));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		FString Message = JsonObject->GetStringField(TEXT("message"));
		UE_LOG(LogD1, Warning, TEXT("Login 실패: %s"), *Message);
		OnLoginResponse.Broadcast(false, Message);
		return;
	}

	AuthToken = JsonObject->GetStringField(TEXT("token"));
	AccountId = (int64)JsonObject->GetNumberField(TEXT("accountId"));
	UE_LOG(LogD1, Log, TEXT("Login 성공: AccountId=%lld"), AccountId);
	OnLoginResponse.Broadcast(true, TEXT(""));
}

void UD1HttpSubsystem::GetCharacters()
{
	TSharedRef<IHttpRequest> Request = MakeRequest(TEXT("GET"), TEXT("/api/characters"));
	Request->OnProcessRequestComplete().BindUObject(this, &UD1HttpSubsystem::OnGetCharactersCompleted);
	Request->ProcessRequest();
}

void UD1HttpSubsystem::OnGetCharactersCompleted(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		UE_LOG(LogD1, Warning, TEXT("GetCharacters 실패"));
		OnGetCharactersResponse.Broadcast(false, TEXT("캐릭터 목록을 불러오지 못했습니다."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		OnGetCharactersResponse.Broadcast(false, TEXT("응답 파싱 실패"));
		return;
	}

	Characters.Empty();
	for (const TSharedPtr<FJsonValue>& Value : JsonArray)
	{
		TSharedPtr<FJsonObject> Obj = Value->AsObject();
		FD1CharacterInfo Info;
		Info.CharacterId = (int64)Obj->GetNumberField(TEXT("characterId"));
		Info.Name        = Obj->GetStringField(TEXT("name"));
		Info.ClassType   = Obj->GetStringField(TEXT("classType"));
		Info.Level       = (int32)Obj->GetNumberField(TEXT("level"));
		Characters.Add(Info);
	}

	UE_LOG(LogD1, Log, TEXT("GetCharacters 성공: %d개"), Characters.Num());
	OnGetCharactersResponse.Broadcast(true, TEXT(""));
}

void UD1HttpSubsystem::CreateCharacter(const FString& Name, const FString& ClassType)
{
	TSharedRef<IHttpRequest> Request = MakeRequest(TEXT("POST"), TEXT("/api/characters"));

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("name"), Name);
	Body->SetStringField(TEXT("classType"), ClassType);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(this, &UD1HttpSubsystem::OnCreateCharacterCompleted);
	Request->ProcessRequest();
}

void UD1HttpSubsystem::OnCreateCharacterCompleted(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnCreateCharacterResponse.Broadcast(false, TEXT("서버에 연결할 수 없습니다."));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, JsonObject);

	if (Response->GetResponseCode() != 201)
	{
		FString Message = JsonObject.IsValid() ? JsonObject->GetStringField(TEXT("message")) : TEXT("캐릭터 생성 실패");
		UE_LOG(LogD1, Warning, TEXT("CreateCharacter 실패: %s"), *Message);
		OnCreateCharacterResponse.Broadcast(false, Message);
		return;
	}

	// 생성 후 목록 갱신
	FD1CharacterInfo Info;
	Info.CharacterId = (int64)JsonObject->GetNumberField(TEXT("characterId"));
	Info.Name        = JsonObject->GetStringField(TEXT("name"));
	Info.ClassType   = JsonObject->GetStringField(TEXT("classType"));
	Info.Level       = (int32)JsonObject->GetNumberField(TEXT("level"));
	Characters.Add(Info);

	UE_LOG(LogD1, Log, TEXT("CreateCharacter 성공: %s (%s)"), *Info.Name, *Info.ClassType);
	OnCreateCharacterResponse.Broadcast(true, TEXT(""));
}

void UD1HttpSubsystem::EnterTown(int64 CharacterId)
{
	TSharedRef<IHttpRequest> Request = MakeRequest(TEXT("POST"), TEXT("/api/matchmaking/town"));

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("characterId"), (double)CharacterId);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(this, &UD1HttpSubsystem::OnEnterTownCompleted);
	Request->ProcessRequest();
}

void UD1HttpSubsystem::OnEnterTownCompleted(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnEnterTownResponse.Broadcast(false, TEXT("서버에 연결할 수 없습니다."));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, JsonObject);

	if (Response->GetResponseCode() != 200)
	{
		FString Message = JsonObject.IsValid() ? JsonObject->GetStringField(TEXT("message")) : TEXT("입장 실패");
		UE_LOG(LogD1, Warning, TEXT("EnterTown 실패: %s"), *Message);
		OnEnterTownResponse.Broadcast(false, Message);
		return;
	}

	const FString ServerAddress = JsonObject->GetStringField(TEXT("serverAddress"));
	const FString SessionToken  = JsonObject->GetStringField(TEXT("sessionToken"));

	// 세션 토큰을 URL 옵션으로 붙여 데디서버 접속. 데디서버가 ParseOption으로 읽음.
	const FString URL = ServerAddress + TEXT("?sessionToken=") + SessionToken;

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogD1, Error, TEXT("EnterTown: LocalPlayerController를 찾을 수 없습니다."));
		OnEnterTownResponse.Broadcast(false, TEXT("접속 컨트롤러를 찾을 수 없습니다."));
		return;
	}

	UE_LOG(LogD1, Log, TEXT("EnterTown 성공: %s 로 접속"), *ServerAddress);
	PC->ClientTravel(URL, ETravelType::TRAVEL_Absolute);
}

void UD1HttpSubsystem::VerifySession(const FString& SessionToken, FD1VerifySessionDelegate OnComplete)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + TEXT("/api/server/sessions/verify"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("X-Server-Api-Key"), ServerApiKey);   // JWT 아님 — 서버 API Key

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("sessionToken"), SessionToken);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	// 요청별 콜백을 람다로 캡처 → 동시 접속 플레이어 간 결과 섞임 방지
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully || !Response.IsValid() || Response->GetResponseCode() != 200)
			{
				const int32 Code = Response.IsValid() ? Response->GetResponseCode() : 0;
				UE_LOG(LogD1Travel, Warning, TEXT("VerifySession 실패 (HTTP %d)"), Code);
				OnComplete.ExecuteIfBound(false, 0, FD1LoadedCharacter());
				return;
			}

			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
			{
				UE_LOG(LogD1Travel, Warning, TEXT("VerifySession: JSON 파싱 실패"));
				OnComplete.ExecuteIfBound(false, 0, FD1LoadedCharacter());
				return;
			}

			const int64 CharacterId = (int64)JsonObject->GetNumberField(TEXT("characterId"));

			FD1LoadedCharacter Data;

			// stats (null이면 신규 캐릭터)
			const TSharedPtr<FJsonObject>* StatsObj = nullptr;
			if (JsonObject->TryGetObjectField(TEXT("stats"), StatsObj) && StatsObj && (*StatsObj).IsValid())
			{
				const TSharedPtr<FJsonObject>& S = *StatsObj;
				Data.Stats.bHasStats       = true;
				Data.Stats.Level           = (int32)S->GetNumberField(TEXT("level"));
				Data.Stats.XP              = (int32)S->GetNumberField(TEXT("xp"));
				Data.Stats.AttributePoints = (int32)S->GetNumberField(TEXT("attributePoints"));
				Data.Stats.SkillPoints     = (int32)S->GetNumberField(TEXT("skillPoints"));
				Data.Stats.Strength        = (float)S->GetNumberField(TEXT("strength"));
				Data.Stats.Intelligence    = (float)S->GetNumberField(TEXT("intelligence"));
				Data.Stats.Dexterity       = (float)S->GetNumberField(TEXT("dexterity"));
				Data.Stats.Luck            = (float)S->GetNumberField(TEXT("luck"));
			}

			// inventory
			const TArray<TSharedPtr<FJsonValue>>* InvArray = nullptr;
			if (JsonObject->TryGetArrayField(TEXT("inventory"), InvArray) && InvArray)
			{
				for (const TSharedPtr<FJsonValue>& V : *InvArray)
				{
					const TSharedPtr<FJsonObject> Obj = V->AsObject();
					if (!Obj.IsValid()) continue;
					FD1LoadedInventoryItem Item;
					Item.SlotIndex   = (int32)Obj->GetNumberField(TEXT("slotIndex"));
					Item.ItemAssetId = Obj->GetStringField(TEXT("itemAssetId"));
					Item.Quantity    = (int32)Obj->GetNumberField(TEXT("quantity"));
					Data.Inventory.Add(Item);
				}
			}

			// equippedItems
			const TArray<TSharedPtr<FJsonValue>>* EquipArray = nullptr;
			if (JsonObject->TryGetArrayField(TEXT("equippedItems"), EquipArray) && EquipArray)
			{
				for (const TSharedPtr<FJsonValue>& V : *EquipArray)
				{
					const TSharedPtr<FJsonObject> Obj = V->AsObject();
					if (!Obj.IsValid()) continue;
					FD1LoadedEquippedItem Item;
					Item.SlotType    = Obj->GetStringField(TEXT("slotType"));
					Item.ItemAssetId = Obj->GetStringField(TEXT("itemAssetId"));
					Data.Equipped.Add(Item);
				}
			}

			// skills
			const TArray<TSharedPtr<FJsonValue>>* SkillArray = nullptr;
			if (JsonObject->TryGetArrayField(TEXT("skills"), SkillArray) && SkillArray)
			{
				for (const TSharedPtr<FJsonValue>& V : *SkillArray)
				{
					const TSharedPtr<FJsonObject> Obj = V->AsObject();
					if (!Obj.IsValid()) continue;
					FD1LoadedSkill Skill;
					Skill.SkillTag   = Obj->GetStringField(TEXT("skillTag"));
					Skill.SkillLevel = (int32)Obj->GetNumberField(TEXT("skillLevel"));
					Data.Skills.Add(Skill);
				}
			}

			// skillSlots (Q/W/E/R)
			const TArray<TSharedPtr<FJsonValue>>* SkillSlotArray = nullptr;
			if (JsonObject->TryGetArrayField(TEXT("skillSlots"), SkillSlotArray) && SkillSlotArray)
			{
				for (const TSharedPtr<FJsonValue>& V : *SkillSlotArray)
				{
					const TSharedPtr<FJsonObject> Obj = V->AsObject();
					if (!Obj.IsValid()) continue;
					FD1LoadedSkillSlot Slot;
					Slot.SlotKey  = Obj->GetStringField(TEXT("slotKey"));
					Slot.SkillTag = Obj->GetStringField(TEXT("skillTag"));
					Data.SkillSlots.Add(Slot);
				}
			}

			UE_LOG(LogD1Travel, Log, TEXT("VerifySession 성공: CharId=%lld, bHasStats=%s, Inv=%d, Equip=%d, Skills=%d, SkillSlots=%d"),
				CharacterId, Data.Stats.bHasStats ? TEXT("TRUE") : TEXT("FALSE"),
				Data.Inventory.Num(), Data.Equipped.Num(), Data.Skills.Num(), Data.SkillSlots.Num());
			OnComplete.ExecuteIfBound(true, CharacterId, Data);
		});

	Request->ProcessRequest();
}

void UD1HttpSubsystem::SaveCharacter(int64 CharacterId, const FString& JsonBody)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + FString::Printf(TEXT("/api/server/characters/%lld/save"), CharacterId));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("X-Server-Api-Key"), ServerApiKey);
	Request->SetContentAsString(JsonBody);   // 본문은 여기서 복사됨 → 이후 PS 소멸과 무관

	// fire-and-forget: 결과 로깅만 (PS/액터 참조하지 않음)
	Request->OnProcessRequestComplete().BindLambda(
		[CharacterId](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			const int32 Code = Response.IsValid() ? Response->GetResponseCode() : 0;
			if (bConnectedSuccessfully && Code == 204)
			{
				UE_LOG(LogD1Travel, Log, TEXT("SaveCharacter 성공: CharId=%lld"), CharacterId);
			}
			else
			{
				UE_LOG(LogD1Travel, Warning, TEXT("SaveCharacter 실패: CharId=%lld (HTTP %d)"), CharacterId, Code);
			}
		});

	Request->ProcessRequest();
}

void UD1HttpSubsystem::IssueSessionToken(int64 CharacterId, const FString& Destination, FD1IssueSessionDelegate OnComplete)
{
	TSharedRef<IHttpRequest> Request = MakeRequest(TEXT("POST"), TEXT("/api/server/sessions/issue"));
	Request->SetHeader(TEXT("X-Server-Api-Key"), ServerApiKey);

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("characterId"), static_cast<double>(CharacterId));
	Body->SetStringField(TEXT("destination"), Destination);

	FString BodyStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyStr);

	Request->OnProcessRequestComplete().BindLambda(
		[CharacterId, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			const int32 Code = Response.IsValid() ? Response->GetResponseCode() : 0;
			if (!bConnectedSuccessfully || Code != 200)
			{
				UE_LOG(LogD1Travel, Warning, TEXT("IssueSessionToken 실패: CharId=%lld (HTTP %d)"), CharacterId, Code);
				OnComplete.ExecuteIfBound(false, FString(), FString());
				return;
			}

			TSharedPtr<FJsonObject> Json;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
			{
				UE_LOG(LogD1Travel, Warning, TEXT("IssueSessionToken: JSON 파싱 실패 CharId=%lld"), CharacterId);
				OnComplete.ExecuteIfBound(false, FString(), FString());
				return;
			}

			const FString Token   = Json->GetStringField(TEXT("sessionToken"));
			const FString Address = Json->GetStringField(TEXT("serverAddress"));
			UE_LOG(LogD1Travel, Log, TEXT("IssueSessionToken 성공: CharId=%lld → %s"), CharacterId, *Address);
			OnComplete.ExecuteIfBound(true, Token, Address);
		});

	Request->ProcessRequest();
}
