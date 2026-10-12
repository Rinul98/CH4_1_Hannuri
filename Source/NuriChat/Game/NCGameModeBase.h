// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NCGameModeBase.generated.h"


class ANCPlayerController;
/**
 * 
 */
UCLASS()
class NURICHAT_API ANCGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void OnPostLogin(AController* NewPlayer) override;
	
	FString GenerateSecretNumber();

	bool IsGuessNumberString(const FString& InNumberString);

	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	virtual void BeginPlay() override;

	void PrintChatMessageString(ANCPlayerController* InChattingPlayerController, const FString& InChatMessageString);

	void IncreaseGuessCount(ANCPlayerController* InChattingPlayerController);

	void ResetGame();

	void JudgeGame(ANCPlayerController* InChattingPlayerController, int InStrikeCount);

	void OnTurnTimerElapsed(); // 타이머가 1초마다 부름

	void MoveToNextTurn(); // 다음 사람에게 차례 넘기기

protected:
	FString SecretNumberString;

	TArray<TObjectPtr<ANCPlayerController>> AllPlayerControllers;

	FTimerHandle TurnTimerHandle;

	UPROPERTY(EditDefaultsOnly)
	int32 TurnTimeLimit = 15; // 한 턴 길이(초)

	int32 CurrentTurnIndex = -1; // AllPlayerControllers에서 지금 차례인 칸 (-1 = 아직 시작 전)

	bool bHasGuessedThisTurn = false; // 활동 체크: 이번 턴에 숫자를 입력했는지
};
