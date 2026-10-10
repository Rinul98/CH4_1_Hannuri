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
		}

		ANCGameStateBase* NCGameStateBase = GetGameState<ANCGameStateBase>();
		if (IsValid(NCGameStateBase) == true)
		{
			NCGameStateBase->MulticastRPCBroadcastLoginMessage(NCPS->PlayerNameString); // 로그인 안내메세지에 플레이어순서이름 넣어주기
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
	Numbers = Numbers.FilterByPredicate([](int32 Num) { return Num > 0; });

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
	FString ChatMessageString = InChatMessageString; //게임진행을 위한 채팅인지, 그냥 채팅인지 체크해야함
	int Index = InChatMessageString.Len() - 3; // 그래서 뒤에서 3개떼섷 확인
	FString GuessNumberString = InChatMessageString.RightChop(Index);

	if (IsGuessNumberString(GuessNumberString) == true) //게스넘버가 맞으면?
	{
		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);

		IncreaseGuessCount(InChattingPlayerController); //시도횟수 증가

		for (TActorIterator<ANCPlayerController> It(GetWorld()); It; ++It)
		{
			ANCPlayerController* NCPlayerController = *It;
			if (IsValid(NCPlayerController) == true)
			{
				FString CombinedMessageString = InChatMessageString + TEXT(" -> ") + JudgeResultString;
				NCPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);

				int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
				JudgeGame(InChattingPlayerController, StrikeCount);
			}
		}
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
		for (const auto& NCPlayerController : AllPlayerControllers)
		{
			if (IsValid(NCPS) == true)
			{
				FString CombinedMessageString = NCPS->PlayerNameString + TEXT(" has won the game.");
				NCPlayerController->NotificationText = FText::FromString(CombinedMessageString);

				ResetGame();
			}
		}
	}
	else
	{
		bool bIsDraw = true;
		for (const auto& NCPlayerController : AllPlayerControllers)
		{
			ANCPlayerState* NCPS = NCPlayerController->GetPlayerState<ANCPlayerState>();
			if (IsValid(NCPS) == true)
			{
				if (NCPS->CurrentGuessCount < NCPS->MaxGuessCount)
				{
					bIsDraw = false;
					break;
				}
			}
		}

		if (true == bIsDraw)
		{
			for (const auto& NCPlayerController : AllPlayerControllers)
			{
				NCPlayerController->NotificationText = FText::FromString(TEXT("Draw..."));

				ResetGame();
			}
		}
	}
}
