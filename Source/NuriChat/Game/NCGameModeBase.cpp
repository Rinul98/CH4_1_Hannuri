// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/NCGameModeBase.h"
#include "NCGameStateBase.h"
#include "Player/NCPlayerController.h"
#include "EngineUtils.h"
#include "Player/NCPlayerState.h"
#include "TimerManager.h"

void ANCGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ANCPlayerController* NCPlayerController = Cast<ANCPlayerController>(NewPlayer);
	if (IsValid(NCPlayerController) == true)
	{

		NCPlayerController->NotificationText = FText::FromString(TEXT("야구게임 시작! 1~9 중 서로 다른 숫자 3개를 맞혀보세요."));

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

	// 1초마다 OnTurnTimerElapsed 호출 (강의 08장 MainTimer 패턴)
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ThisClass::OnTurnTimerElapsed, 1.0f, true);
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
		// 승리/무승부 후 재시작 대기 중이면 받지 않음
		if (GetWorldTimerManager().IsTimerActive(RestartTimerHandle) == true)
		{
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("곧 새 게임이 시작됩니다. 잠시만 기다려 주세요."));
			return;
		}

		// 보낸 사람의 PlayerState를 꺼내서 기회가 남았는지 확인 (서버에서 막아야 클라 조작으로 못 뚫음)
		ANCPlayerState* ChattingPlayerState = InChattingPlayerController->GetPlayerState<ANCPlayerState>();
		if (IsValid(ChattingPlayerState) == true && ChattingPlayerState->CurrentGuessCount >= ChattingPlayerState->MaxGuessCount)
		{
			// 보낸 사람에게만 안내 (Client RPC는 그 컨트롤러 주인 클라에서만 실행됨)
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("기회를 모두 사용했습니다. 다음 게임을 기다려 주세요."));
			return; // 판정, 시도 횟수 증가, 전체 방송 모두 하지 않고 끝냄
		}

		// 턴 검사: 내 차례가 아니거나, 이번 턴에 이미 입력했거나, 0초면 서버가 받지 않음
		ANCGameStateBase* NCGameStateBase = GetGameState<ANCGameStateBase>();
		bool bIsMyTurn = AllPlayerControllers.IsValidIndex(CurrentTurnIndex) == true && AllPlayerControllers[CurrentTurnIndex] == InChattingPlayerController;
		if (bIsMyTurn == false || bHasGuessedThisTurn == true || (IsValid(NCGameStateBase) == true && NCGameStateBase->RemainingTurnTime <= 0))
		{
			FString TurnPlayerName = IsValid(NCGameStateBase) == true ? NCGameStateBase->CurrentTurnPlayerName : FString();
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("지금은 입력할 수 없습니다. (현재 차례: ") + TurnPlayerName + TEXT(")"));
			return;
		}
		bHasGuessedThisTurn = true; // 활동 체크: 이번 턴에 입력함 → 다음 타이머 때 차례가 넘어감

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

	CurrentTurnIndex = -1; // 다음 턴 타이머 때 "야구게임 시작!" 공지 + 첫 차례부터 다시 시작
}

void ANCGameModeBase::JudgeGame(ANCPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (3 == InStrikeCount) // 승리
	{
		ANCPlayerState* NCPS = InChattingPlayerController->GetPlayerState<ANCPlayerState>();
		if (IsValid(NCPS) == true)
		{
			FString CombinedMessageString = NCPS->PlayerNameString + FString::Printf(TEXT(" 님이 승리했습니다! %d초 후 재시작됩니다."), FMath::RoundToInt(RestartDelay));
			for (const auto& NCPlayerController : AllPlayerControllers)
			{
				NCPlayerController->NotificationText = FText::FromString(CombinedMessageString); // 알림은 모두에게
			}
		}

		// RestartDelay초 뒤에 한 번만 재시작 (기다리는 동안 입력과 턴 시간은 멈춤)
		GetWorldTimerManager().SetTimer(RestartTimerHandle, this, &ThisClass::ResetGame, RestartDelay, false);
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
				NCPlayerController->NotificationText = FText::FromString(FString::Printf(TEXT("무승부입니다. %d초 후 재시작됩니다."), FMath::RoundToInt(RestartDelay)));
			}

			GetWorldTimerManager().SetTimer(RestartTimerHandle, this, &ThisClass::ResetGame, RestartDelay, false);
		}
	}
}

void ANCGameModeBase::OnTurnTimerElapsed()
{
	ANCGameStateBase* NCGameStateBase = GetGameState<ANCGameStateBase>();
	if (IsValid(NCGameStateBase) == false || AllPlayerControllers.Num() == 0)
	{
		return; // 아직 아무도 없으면 시간 안 흐름
	}

	if (GetWorldTimerManager().IsTimerActive(RestartTimerHandle) == true)
	{
		return; // 재시작 대기 중에는 턴 시간 멈춤
	}

	// 첫 접속 후 처음 도는 타이머면 첫 턴 시작
	if (CurrentTurnIndex == -1)
	{
		for (const auto& NCPlayerController : AllPlayerControllers)
		{
			NCPlayerController->NotificationText = FText::FromString(TEXT("야구게임 시작! 1~9 중 서로 다른 숫자 3개를 맞혀보세요."));
		}

		MoveToNextTurn();
		return;
	}

	// 이번 턴에 숫자를 입력했으면 다음 사람 차례
	if (bHasGuessedThisTurn == true)
	{
		MoveToNextTurn();
		return;
	}

	NCGameStateBase->RemainingTurnTime--;
	if (NCGameStateBase->RemainingTurnTime > 0)
	{
		return;
	}

	// 입력 없이 시간이 끝남 → 기회 1회 차감
	if (AllPlayerControllers.IsValidIndex(CurrentTurnIndex) == true)
	{
		ANCPlayerController* TurnPlayerController = AllPlayerControllers[CurrentTurnIndex];
		if (IsValid(TurnPlayerController) == true)
		{
			IncreaseGuessCount(TurnPlayerController);
			TurnPlayerController->ClientRPCPrintChatMessageString(TEXT("시간 초과! 기회 1회가 차감됩니다."));
			JudgeGame(TurnPlayerController, 0); // 모두 기회를 다 썼으면 무승부 처리
		}
	}

	MoveToNextTurn();
}

void ANCGameModeBase::MoveToNextTurn()
{
	ANCGameStateBase* NCGameStateBase = GetGameState<ANCGameStateBase>();
	if (IsValid(NCGameStateBase) == false || AllPlayerControllers.Num() == 0)
	{
		return;
	}

	// 다음 칸부터 한 바퀴 돌면서 기회가 남은 사람을 찾음 (다 쓴 사람, 나간 사람은 건너뜀)
	for (int32 Step = 1; Step <= AllPlayerControllers.Num(); ++Step)
	{
		int32 NextIndex = (CurrentTurnIndex + Step) % AllPlayerControllers.Num();
		ANCPlayerController* NextPlayerController = AllPlayerControllers[NextIndex];
		if (IsValid(NextPlayerController) == false)
		{
			continue;
		}

		ANCPlayerState* NextPlayerState = NextPlayerController->GetPlayerState<ANCPlayerState>();
		if (IsValid(NextPlayerState) == true && NextPlayerState->CurrentGuessCount < NextPlayerState->MaxGuessCount)
		{
			CurrentTurnIndex = NextIndex;
			NCGameStateBase->CurrentTurnPlayerName = NextPlayerState->PlayerNameString;
			break;
		}
	}

	NCGameStateBase->RemainingTurnTime = TurnTimeLimit; // 새 턴 시간 채우기
	bHasGuessedThisTurn = false; // 활동 체크 초기화
}
