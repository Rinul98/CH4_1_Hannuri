// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NCPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class NURICHAT_API ANCPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ANCPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FString GetPlayerInfoString(); // 시도횟수 출력

public:
	UPROPERTY(Replicated) // 속성 키워드 지정
	FString PlayerNameString;

	UPROPERTY(Replicated) // 시도획수
	int32 CurrentGuessCount;

	UPROPERTY(Replicated)
	int32 MaxGuessCount;

};
