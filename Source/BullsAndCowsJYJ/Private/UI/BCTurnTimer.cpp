//BCTurnTimer.cpp


#include "UI/BCTurnTimer.h"

#include "Components/TextBlock.h"
#include "Game/BCGameStateBase.h"

void UBCTurnTimer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	//부모(UUserWidget)의 NativeTick을 먼저 실행함 (위젯 애니메이션 처리, 블루프린트의 Event Tick 호출)
	Super::NativeTick(MyGeometry, InDeltaTime);

	//남은 시간은 서버가 GameState에 넣어 복제해 준 값이다. 위젯은 그걸 읽어서 보여 주기만 함
	//(WBP_NotficationText가 바인딩 함수로 NotificationText를 매 프레임 읽는 것과 같은 방식)
	//GetWorld()->GetGameState<T>(): 이 월드의 GameState를 T로 Cast해서 준다. GameState는 클라이언트에도 복제돼 있어서 클라에서도 얻을 수 있음
	ABCGameStateBase* BCGS = GetWorld()->GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == false)
	{
		return;
	}

	//0이면 턴이 없는 상태(결과 공지 중)라서 글자를 비움
	if (BCGS->RemainingTurnTime > 0)
	{
		//게임 중에는 기회 줄도 보여 줌. 대기 중에 접었던 줄을 다시 켜야 두 줄이 같이 나옴
		TextBlock_RemainingChance->SetVisibility(ESlateVisibility::HitTestInvisible);
		//%s 자리에 FString을 넣을 때는 앞에 *를 붙여 TCHAR* 문자열로 바꿔 넘김. %d는 정수
		//TextBlock의 SetText는 FText를 받으니 FText::FromString으로 바꿔 넣음
		//왼쪽 전광판은 차례·기회, 오른쪽은 남은 시간으로 보여 줌
		TextBlock_TurnPlayer->SetText(FText::FromString(FString::Printf(TEXT("%s 차례"), *BCGS->CurrentTurnPlayerNameString)));
		TextBlock_RemainingChance->SetText(FText::FromString(FString::Printf(TEXT("기회 %d번 남음"), BCGS->CurrentTurnRemainingGuessCount)));
		TextBlock_TurnTimer->SetText(FText::FromString(FString::Printf(TEXT("%d초"), BCGS->RemainingTurnTime)));
	}
	else
	{
		//상대를 기다리는 중(Waiting)이면 왼쪽 위 전광판에 "대기 중"을 띄운다. 결과 공지 중·매치 표시 중에는 비워 둔다
		//삼항 연산자: 조건이 참이면 앞, 거짓이면 뒤 값을 고른다
		TextBlock_TurnPlayer->SetText(BCGS->MatchState == EBCMatchState::Waiting ? FText::FromString(TEXT("대기 중")) : FText::GetEmpty());
		TextBlock_RemainingChance->SetText(FText::GetEmpty());
		//글자만 비우면 줄 높이는 남아 있음. 줄 자체를 접어야 '대기 중' 한 줄이 전광판 가운데에 옴
		TextBlock_RemainingChance->SetVisibility(ESlateVisibility::Collapsed);
		TextBlock_TurnTimer->SetText(FText::GetEmpty());
	}
}
