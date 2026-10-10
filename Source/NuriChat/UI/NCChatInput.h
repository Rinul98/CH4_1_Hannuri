// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "NCChatInput.generated.h"

class UEditableTextBox;

/**
 * 
 */
UCLASS()
class NURICHAT_API UNCChatInput : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override; // 위젯이 화면에 만들어질 때, 사라질 때 자동으로 불러줌

	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod); // 텍스트 박스 신호 받음 UFUNCtion 필수 << 







public:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;

	
};
