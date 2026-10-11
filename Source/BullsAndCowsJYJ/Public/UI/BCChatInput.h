
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BCChatInput.generated.h"

class UEditableTextBox;
class UScrollBox;
class UTextBlock;

//채팅창(태블릿) 위젯의 C++ 부모 클래스. 화면 배치(디자인)는 이 클래스를 상속한 WBP에서 하고, 동작은 여기 C++에서 함
//입력창(EditableTextBox_ChatInput)과 채팅 기록(ScrollBox_ChatLog 안의 TextBlock_ChatLog)을 함께 맡는다
UCLASS()
class BULLSANDCOWSJYJ_API UBCChatInput : public UUserWidget
{
	GENERATED_BODY()

public:
	//meta=(BindWidget): WBP 안에 이름(EditableTextBox_ChatInput)과 타입이 같은 위젯이 있으면 이 포인터에 자동으로 연결됨
	//이름이 다르거나 없으면 WBP를 컴파일할 때 에러가 난다
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;

	//채팅 기록을 담는 스크롤 상자. 줄이 많아져 칸을 넘치면 스크롤됨
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_ChatLog;

	//채팅 기록 글자. WBP에서 ScrollBox_ChatLog 안에 넣음. 새 줄을 받을 때마다 아래에 이어 붙임
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_ChatLog;

	//위젯이 화면에 붙어 만들어질 때 불림 (블루프린트의 Event Construct). 뗐다가 다시 붙이면 또 불림
	virtual void NativeConstruct() override;
	//위젯이 화면에서 떨어져 정리될 때 불림 (블루프린트의 Event Destruct)
	virtual void NativeDestruct() override;

	//채팅 한 줄을 기록 맨 아래에 붙이고 맨 아래로 스크롤함. PC의 PrintChatMessageString이 부름
	//받은 채팅은 여기 TextBlock에 한 줄씩 이어 붙임
	void AddChatMessage(const FString& InChatMessageString);

protected:
	//입력창의 OnTextCommitted 이벤트에 연결할 함수
	//UFUNCTION(): AddDynamic으로 연결하는 다이내믹 델리게이트는 함수를 "이름"으로 찾기 때문에 리플렉션에 등록된 UFUNCTION이어야 함
	//매개변수 모양(const FText&, ETextCommit::Type)이 OnTextCommitted 델리게이트와 똑같아야 연결됨
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

};
