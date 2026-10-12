//BCScoreBoard.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateColor.h"
#include "BCScoreBoard.generated.h"

class UTextBlock;

//가운데 전광판 위젯의 C++ 부모 클래스 (WBP_ScoreBoard가 상속)
//마지막으로 판정한 숫자를 크게("321"), 그 결과를 아래에 작게("1S2B" / "OUT") 보여 줌
//WBP_ScoreBoard에는 야구장 배경 그림도 같이 넣고, PC가 맨 아래(ZOrder 0)에 깔아서 다른 위젯들이 그 위에 그려지게 함
UCLASS()
class BULLSANDCOWSJYJ_API UBCScoreBoard : public UUserWidget
{
	GENERATED_BODY()

public:
	//전광판 큰 글씨: 마지막으로 입력된 숫자 (예: "321")
	//BindWidget: WBP에 이름과 타입이 똑같은 TextBlock이 있어야 함. 없으면 WBP 컴파일 에러
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_GuessNumber;

	//전광판 작은 글씨: 그 숫자의 판정 결과 (예: "1S2B", "OUT")
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_GuessResult;

	//기존 채팅 Client RPC에 같이 실려 온 판정 사건으로 효과음·강조 연출을 시작함. 같은 숫자를 연속 입력해도 매번 실행됨
	void ShowGuessFeedback(const FString& InGuessResultString);

protected:
	virtual void NativeConstruct() override;
	//0.6초 연출의 경과 시간만 기억함. 판정 숫자와 결과는 기존 GameState 값을 계속 사용함
	float FeedbackElapsedTime = 0.6f;
	FSlateColor DefaultResultColor;

	//매 프레임 GameState의 LastGuessNumberString, LastGuessResultString을 읽어 글자를 바꿈 (UBCTurnTimer와 같은 방식)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
