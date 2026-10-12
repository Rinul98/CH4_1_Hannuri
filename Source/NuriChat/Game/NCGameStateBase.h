// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NCGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class NURICHAT_API ANCGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCBroadcastLoginMessage(const FString& InNameString = FString(TEXT("XXXXXXX"))); // 접속할때 메세지

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 남은 턴 시간(초). 서버가 줄이고 모든 클라에 복제됨 → 위젯이 바인딩해서 표시
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 RemainingTurnTime = 0;

	// 지금 차례인 플레이어 이름 (위젯 표시용)
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString CurrentTurnPlayerName;
};
