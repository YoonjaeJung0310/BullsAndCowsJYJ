

#include "UI/BCChatInput.h"

#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Player/BCPlayerController.h"

void UBCChatInput::NativeConstruct()
{
	//부모(UUserWidget)의 NativeConstruct를 먼저 실행함 (블루프린트의 Event Construct도 여기서 불림)
	Super::NativeConstruct();

	//OnTextCommitted(입력 확정 이벤트)에 우리 함수를 연결함
	//NativeConstruct는 여러 번 불릴 수 있음. 같은 함수를 두 번 연결하면 엔터 한 번에 두 번 불리므로, IsAlreadyBound로 이미 연결돼 있는지 먼저 확인함
	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false)
	{
		//AddDynamic(누구의, 어떤 함수): 다이내믹 델리게이트에 함수를 연결함
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}

}

void UBCChatInput::NativeDestruct()
{
	Super::NativeDestruct();

	//Construct에서 연결한 것을 Destruct에서 끊어 짝을 맞춘다 → 다시 Construct될 때 깨끗한 상태에서 한 번만 연결됨
	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == true)
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

//입력창에서 글자 입력이 "확정"될 때마다 불림 (엔터, 포커스 이동 등)
void UBCChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	//CommitMethod: 어떻게 확정됐는가. Default / OnEnter(엔터) / OnUserMovedFocus(Tab·클릭으로 포커스 이동) / OnCleared(Esc 등)
	//엔터로 확정했을 때만 보낸다 → 입력창 밖을 클릭해서 포커스가 빠질 때는 전송되지 않는다
	if (CommitMethod == ETextCommit::OnEnter)
	{
		//빈칸이거나 공백만 친 엔터는 서버로 보내지 않는다 → 남의 차례에 "Player2 [0/3]: " 같은 빈 채팅 줄이 모두에게 방송되던 것을 막는다
		//IsEmptyOrWhitespace: 글자가 없거나 공백 문자뿐이면 true
		if (Text.IsEmptyOrWhitespace() == true)
		{
			return;
		}

		//GetOwningPlayer(): 이 위젯의 주인 PC. BCPlayerController의 BeginPlay에서 CreateWidget(this, ...)로 넘긴 그 PC다
		APlayerController* OwningPlayerController = GetOwningPlayer();
		if (IsValid(OwningPlayerController) == true)
		{
			ABCPlayerController* OwningBCPlayerController = Cast<ABCPlayerController>(OwningPlayerController);
			if (IsValid(OwningBCPlayerController) == true)
			{
				//FText → FString으로 바꿔서 PC에 넘김. 여기부터 PC가 서버로 보내는 일을 맡는다
				OwningBCPlayerController->SetChatMessageString(Text.ToString());

				//빈 FText를 넣어 입력창을 비움
				EditableTextBox_ChatInput->SetText(FText());
			}
		}
	}
}

void UBCChatInput::AddChatMessage(const FString& InChatMessageString)
{
	//지금까지의 기록 뒤에 줄바꿈(\n)과 새 줄을 붙임. 기록이 비어 있으면(첫 줄) 줄바꿈 없이 바로 씀
	//기록을 따로 변수에 들고 있지 않고 TextBlock에 이미 들어 있는 글자를 꺼내 이어 붙임
	FString ChatLogString = TextBlock_ChatLog->GetText().ToString();
	if (ChatLogString.IsEmpty() == false)
	{
		ChatLogString += TEXT("\n");
	}
	ChatLogString += InChatMessageString;
	TextBlock_ChatLog->SetText(FText::FromString(ChatLogString));

	//새 줄이 보이게 맨 아래로 스크롤함. 줄이 칸을 넘치지 않을 때는 아무 일도 없음
	ScrollBox_ChatLog->ScrollToEnd();
}
