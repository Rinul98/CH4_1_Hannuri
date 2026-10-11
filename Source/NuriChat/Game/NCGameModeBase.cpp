// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/NCGameModeBase.h"
#include "NCGameStateBase.h"
#include "Player/NCPlayerController.h"
#include "EngineUtils.h"
#include "Player/NCPlayerState.h"

void ANCGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ANCPlayerController* NCPlayerController = Cast<ANCPlayerController>(NewPlayer);
	if (IsValid(NCPlayerController) == true)
	{

		NCPlayerController->NotificationText = FText::FromString(TEXT("Connected to the game server."));

		AllPlayerControllers.Add(NCPlayerController);

		ANCPlayerState* NCPS = NCPlayerController->GetPlayerState<ANCPlayerState>();
		if (IsValid(NCPS) == true)
		{
			NCPS->PlayerNameString = TEXT("Player") + FString::FromInt(AllPlayerControllers.Num());

			// NCPS가 유효할 때만 이름을 쓸 수 있으니 검사 안으로 옮김
			ANCGameStateBase* NCGameStateBase = GetGameState<ANCGameStateBase>();
			if (IsValid(NCGameStateBase) == true)
			{
				NCGameStateBase->MulticastRPCBroadcastLoginMessage(NCPS->PlayerNameString); // 로그인 안내메세지에 플레이어순서이름 넣어주기
			}
		}
	}
}

FString ANCGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	return Result;
}

bool ANCGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	bool bCanPlay = false;

	do {

		if (InNumberString.Len() != 3) // 자리수 3개 아니면 막기
		{
			break;
		}

		bool bIsUnique = true;
		TSet<TCHAR> UniqueDigits;
		for (TCHAR C : InNumberString)
		{
			if (FChar::IsDigit(C) == false || C == '0')
			{
				bIsUnique = false;
				break;
			}

			UniqueDigits.Add(C);
		}

		if (bIsUnique == false)
		{
			break;
		}

		if (UniqueDigits.Num() != 3) // TSet은 중복을 안 받으니, 3개가 안 되면 중복 처리
		{
			break;
		}

		bCanPlay = true;

	} while (false);

	return bCanPlay;
}

FString ANCGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i]) // 자리랑 숫자 똑같으면 스트라이크 
		{
			StrikeCount++;
		}
		else
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]); // 포함되어 있는데 자리 틀리면 볼
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ANCGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Error, TEXT("%s"), *SecretNumberString)
}

void ANCGameModeBase::PrintChatMessageString(ANCPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	// 클라가 보낸 문자열은 "Player1(1/3): 입력" 모양이라, ": " 뒤의 실제 입력만 꺼냄
	FString PlayerInfoString;
	FString GuessNumberString;
	if (InChatMessageString.Split(TEXT(": "), &PlayerInfoString, &GuessNumberString) == false)
	{
		GuessNumberString = InChatMessageString; // ": "가 없으면 전체를 입력으로 봄
	}
	GuessNumberString.TrimStartAndEndInline(); // 앞뒤 공백 제거

	// 숫자가 하나라도 있으면 게임 입력을 하려던 것으로 봄
	bool bHasDigit = false;
	for (TCHAR Character : GuessNumberString)
	{
		if (FChar::IsDigit(Character) == true)
		{
			bHasDigit = true;
			break;
		}
	}

	// 게임 입력인데 규칙에 안 맞으면 보낸 사람에게만 안내하고 끝냄 기회 소진 X
	if (bHasDigit == true && IsGuessNumberString(GuessNumberString) == false)
	{
		InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("다시 입력하세요. (1~9 중 서로 다른 숫자 3개)"));
		return;
	}

	if (IsGuessNumberString(GuessNumberString) == true) //게스넘버가 맞으면?
	{
		// 보낸 사람의 PlayerState를 꺼내서 기회가 남았는지 확인 (서버에서 막아야 클라 조작으로 못 뚫음)
		ANCPlayerState* ChattingPlayerState = InChattingPlayerController->GetPlayerState<ANCPlayerState>();
		if (IsValid(ChattingPlayerState) == true && ChattingPlayerState->CurrentGuessCount >= ChattingPlayerState->MaxGuessCount)
		{
			// 보낸 사람에게만 안내 (Client RPC는 그 컨트롤러 주인 클라에서만 실행됨)
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("기회를 모두 사용했습니다. 다음 게임을 기다려 주세요."));
			return; // 판정, 시도 횟수 증가, 전체 방송 모두 하지 않고 끝냄
		}

		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);

		IncreaseGuessCount(InChattingPlayerController); //시도횟수 증가

		// 클라가 붙인 이름표는 늘리기 전 횟수라서, 서버가 늘린 뒤 횟수로 다시 만듦
		FString SenderInfoString = PlayerInfoString;
		if (IsValid(ChattingPlayerState) == true)
		{
			SenderInfoString = ChattingPlayerState->GetPlayerInfoString();
		}
		FString CombinedMessageString = SenderInfoString + TEXT(": ") + GuessNumberString + TEXT(" -> ") + JudgeResultString;

		for (TActorIterator<ANCPlayerController> It(GetWorld()); It; ++It)
		{
			ANCPlayerController* NCPlayerController = *It;
			if (IsValid(NCPlayerController) == true)
			{
				NCPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString); // 반복문은 방송만
			}
		}

		// 판정은 반복문 밖에서 한 번만 (안에 있으면 플레이어 수만큼 재시작됨)
		int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1)); // "OUT"이면 0
		JudgeGame(InChattingPlayerController, StrikeCount);
	}
	else
	{
		for (TActorIterator<ANCPlayerController> It(GetWorld()); It; ++It)
		{
			ANCPlayerController* NCPlayerController = *It;
			if (IsValid(NCPlayerController) == true)
			{
				NCPlayerController->ClientRPCPrintChatMessageString(InChatMessageString); //게임하려고 보낸게 아니면 그냥 보내버리기
			}
		}
	}
}

void ANCGameModeBase::IncreaseGuessCount(ANCPlayerController* InChattingPlayerController)
{
	ANCPlayerState* NCPS = InChattingPlayerController->GetPlayerState<ANCPlayerState>();
	if (IsValid(NCPS) == true)
	{
		NCPS->CurrentGuessCount++;
	}
}

void ANCGameModeBase::ResetGame() // 정답숫자 바꿔주고 게스카운트 초기화
{
	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Error, TEXT("새 정답: %s"), *SecretNumberString); // 시연/테스트용 정답 확인

	for (const auto& NCPlayerController : AllPlayerControllers)
	{
		ANCPlayerState* NCPS = NCPlayerController->GetPlayerState<ANCPlayerState>();
		if (IsValid(NCPS) == true)
		{
			NCPS->CurrentGuessCount = 0;
		}
	}
}

void ANCGameModeBase::JudgeGame(ANCPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (3 == InStrikeCount) // 승리
	{
		ANCPlayerState* NCPS = InChattingPlayerController->GetPlayerState<ANCPlayerState>();
		if (IsValid(NCPS) == true)
		{
			FString CombinedMessageString = NCPS->PlayerNameString + TEXT(" 님이 승리했습니다!");
			for (const auto& NCPlayerController : AllPlayerControllers)
			{
				NCPlayerController->NotificationText = FText::FromString(CombinedMessageString); // 알림은 모두에게
			}
		}

		ResetGame(); // 반복문 밖에서 한 번만 재시작
	}
	else
	{
		bool bIsDraw = true;
		for (const auto& NCPlayerController : AllPlayerControllers)
		{
			ANCPlayerState* NCPS = NCPlayerController->GetPlayerState<ANCPlayerState>();
			if (IsValid(NCPS) == true && NCPS->CurrentGuessCount < NCPS->MaxGuessCount)
			{
				bIsDraw = false; // 기회 남은 사람이 한 명이라도 있으면 아직 진행 중
				break;
			}
		}

		if (true == bIsDraw)
		{
			for (const auto& NCPlayerController : AllPlayerControllers)
			{
				NCPlayerController->NotificationText = FText::FromString(TEXT("무승부입니다. 새 게임을 시작합니다."));
			}

			ResetGame(); // 반복문 밖에서 한 번만 재시작
		}
	}
}
