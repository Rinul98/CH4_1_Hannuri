// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NCPlayerController.generated.h"


class UNCChatInput;
class UUserWidget;


/**
 * 
 */
UCLASS()
class NURICHAT_API ANCPlayerController : public APlayerController
{
	GENERATED_BODY()


public:

	ANCPlayerController();

	virtual void BeginPlay() override;

	void SetChatMessageString(const FString& InChatMessageString); // 이 함수 불러서 문장 넘기기

	void PrintChatMessageString(const FString& InChatMessageString); 

	UFUNCTION(Client, Reliable) // 클라이언트 rpc
	void ClientRPCPrintChatMessageString(const FString& InChatMessageString);

	UFUNCTION(Server, Reliable) // 서버 rpc
	void ServerRPCPrintChatMessageString(const FString& InChatMessageString);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UNCChatInput> ChatInputWidgetClass;

	UPROPERTY()
	TObjectPtr<UNCChatInput> ChatInputWidgetInstance;

	FString ChatMessageString; // 마지막으로 입력한 채팅 문장을 담는 변수

	UPROPERTY(EditDefaultsOnly) // 공지사항
	TSubclassOf<UUserWidget> NotificationTextWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> NotificationTextWidgetInstance;

public: // 공지사항
	UPROPERTY(Replicated, BlueprintReadOnly)
	FText NotificationText;
	
};
