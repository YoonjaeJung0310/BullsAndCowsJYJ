//BCWaitingScreen.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BCWaitingScreen.generated.h"

class UButton;
class UTextBlock;

//대기 화면 위젯의 C++ 부모 클래스 (WBP_WaitingScreen이 상속)
//GameState의 MatchState를 보고: Waiting = "상대를 기다리는 중...", Matched = "매치되었습니다!", Playing = 숨김
UCLASS()
class BULLSANDCOWSJYJ_API UBCWaitingScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	//게임 화면을 덮는 판(반투명 Border 등). 게임 중(Playing)에는 이 판만 숨긴다
	//위젯 자신(루트)을 숨기면 화면에 그려지지 않아 NativeTick도 불리지 않는다 → 다시 보이게 할 수 없으니 안쪽 판만 켜고 끈다
	//UWidget으로 받으므로 Border, Overlay, CanvasPanel 등 어떤 위젯이든 이름만 맞으면 연결됨
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> Panel_Waiting;

	//"상대를 기다리는 중..." / "매치되었습니다!"를 띄우는 글자. Panel_Waiting 안에 넣음
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_WaitingMessage;

	//대기 문구 아래의 메인 메뉴 버튼. WBP에서도 이 이름으로 만들어야 실제 버튼 객체가 자동 연결됨
	//Panel_Waiting 안에 넣어 게임이 시작되어 판이 숨겨질 때 버튼도 함께 숨긴다
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_MainMenu;

protected:
	//위젯이 화면에 구성될 때 메인 메뉴 버튼의 클릭 이벤트를 연결함
	//NativeConstruct는 다시 불릴 수 있으므로 구현에서 AddUniqueDynamic으로 중복 연결을 막는다
	virtual void NativeConstruct() override;

	//매 프레임 GameState의 MatchState를 읽어 판을 켜고 끄고 문구를 바꿈 (UBCTurnTimer와 같은 방식)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//위젯은 클릭을 받고, 실제 서버 접속 종료와 레벨 이동은 주인 PlayerController에게 맡긴다
	//다이내믹 델리게이트가 이 함수를 이름으로 찾으므로 UFUNCTION이 필요하다
	UFUNCTION()
	void OnMainMenuButtonClicked();
};
