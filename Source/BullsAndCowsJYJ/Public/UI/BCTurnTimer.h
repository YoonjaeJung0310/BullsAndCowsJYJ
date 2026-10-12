//BCTurnTimer.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateColor.h"
#include "BCTurnTimer.generated.h"

class UTextBlock;
class UButton;

//턴 정보를 보여 주는 위젯의 C++ 부모 클래스 (WBP_TurnTimer가 상속)
//왼쪽 위 전광판: "누구 차례" + "기회 N번 남음" / 오른쪽 위: 남은 시간
UCLASS()
class BULLSANDCOWSJYJ_API UBCTurnTimer : public UUserWidget
{
	GENERATED_BODY()

public:
	//WBP에 이름이 똑같은(TextBlock_TurnTimer) TextBlock이 있어야 함. 없으면 WBP 컴파일 에러 (BindWidget)
	//오른쪽 위 타이머: "30초"
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_TurnTimer;

	//왼쪽 위 전광판 윗줄: "미르 차례"
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_TurnPlayer;

	//왼쪽 위 전광판 아랫줄: "기회 2번 남음" (지금 차례인 사람의 남은 기회)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_RemainingChance;

	//왼쪽 전광판 아래 메뉴 복귀 버튼. WBP_TurnTimer에도 같은 이름의 Button이 있어야 함
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_MainMenu;

protected:
	//평소 색을 기억해 마지막 5초의 경고 색을 새 턴에서 원래대로 돌림
	virtual void NativeConstruct() override;
	FSlateColor DefaultTimerColor;
	//Tick마다 같은 초의 소리가 반복되지 않도록 직전 표시 상태만 기억함. 서버 시간을 바꾸는 값은 아님
	FString PreviousTurnPlayerName;
	int32 PreviousRemainingTime = 0;

	//위젯이 화면에 있는 동안 매 프레임 불림 (블루프린트의 Event Tick). InDeltaTime = 지난 프레임부터 흐른 시간(초)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//버튼은 이동 요청만 보냄. 접속 종료와 메뉴 주소 처리는 기존 PC 함수가 맡음
	UFUNCTION()
	void OnMainMenuButtonClicked();
};
