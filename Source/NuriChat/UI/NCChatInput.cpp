// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NCChatInput.h"
#include "Components/EditableTextBox.h"
#include "Player/NCPlayerController.h"

void UNCChatInput::NativeConstruct() // 텍스트박스 신호에 내 함수 연결 (.adddynamic)
{
	Super::NativeConstruct();

		if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false) // 이미 연결되어있는지 검사
		{
			EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);
		}
}

void UNCChatInput::NativeDestruct() // 위젯 사라질때 연결 끊기
{
	Super::NativeDestruct();

		if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == true)
		{
			EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
		}
}

void UNCChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{

	if (CommitMethod == ETextCommit::OnEnter) // 포커스 빠지거나 tab말고 Enter로 했을때만 처리
	{
		APlayerController* OwningPlayerController = GetOwningPlayer();
		if (IsValid(OwningPlayerController) == true)
		{
			ANCPlayerController* OwningNCPlayerController = Cast<ANCPlayerController>(OwningPlayerController); 
			if (IsValid(OwningNCPlayerController) == true)
			{
				OwningNCPlayerController->SetChatMessageString(Text.ToString()); // 방금 친 문장 보관 문자열로 바꿔서 넘기기

				EditableTextBox_ChatInput->SetText(FText()); // 입력칸을 빈칸으로 지우기
			}
		}
	}
}
