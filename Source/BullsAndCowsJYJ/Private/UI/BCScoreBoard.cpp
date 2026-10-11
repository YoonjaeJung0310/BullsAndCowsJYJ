//BCScoreBoard.cpp


#include "UI/BCScoreBoard.h"

#include "Components/TextBlock.h"
#include "Game/BCGameStateBase.h"

void UBCScoreBoard::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//판정 결과는 서버(GameMode)가 GameState에 넣어 복제해 준 값이다. 위젯은 읽어서 보여 주기만 함
	ABCGameStateBase* BCGS = GetWorld()->GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == false)
	{
		return;
	}

	//새 판이 시작되면 GameMode가 두 값을 비우므로(ResetGame) 전광판도 자연히 빈칸이 됨
	TextBlock_GuessNumber->SetText(FText::FromString(BCGS->LastGuessNumberString));
	TextBlock_GuessResult->SetText(FText::FromString(BCGS->LastGuessResultString));
}
