//BCScoreBoard.cpp


#include "UI/BCScoreBoard.h"

#include "Components/TextBlock.h"
#include "Game/BCGameStateBase.h"
#include "Audio/BCAudioSubsystem.h"

void UBCScoreBoard::NativeConstruct()
{
	Super::NativeConstruct();
	DefaultResultColor = TextBlock_GuessResult->GetColorAndOpacity();
}

void UBCScoreBoard::ShowGuessFeedback(const FString& InGuessResultString)
{
	//복제 값의 변화만 검사하면 같은 숫자·같은 결과를 연속 입력했을 때 사건을 놓침. 입력 한 번마다 오는 RPC를 사용함
	FeedbackElapsedTime = 0.0f;
	UBCAudioSubsystem::PlayEffect(this, InGuessResultString == TEXT("OUT") ? TEXT("S_Out") : TEXT("S_Result"));
}

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

	//배경 그림까지 커지지 않도록 글자 두 개에만 짧은 확대·복귀 연출을 줌
	FeedbackElapsedTime = FMath::Min(FeedbackElapsedTime + InDeltaTime, 0.6f);
	const float Emphasis = FMath::Sin(FeedbackElapsedTime / 0.6f * PI);
	const float TextScale = 1.0f + 0.12f * Emphasis;
	TextBlock_GuessNumber->SetRenderScale(FVector2D(TextScale, TextScale));
	TextBlock_GuessResult->SetRenderScale(FVector2D(TextScale, TextScale));
	//3S일 때만 결과 글씨도 기존 숫자의 노란색 쪽으로 잠깐 밝아졌다가 원래 빨간색으로 돌아옴
	TextBlock_GuessResult->SetColorAndOpacity(BCGS->LastGuessResultString == TEXT("3S0B")
		? FSlateColor(FMath::Lerp(DefaultResultColor.GetSpecifiedColor(), TextBlock_GuessNumber->GetColorAndOpacity().GetSpecifiedColor(), Emphasis))
		: DefaultResultColor);
}
