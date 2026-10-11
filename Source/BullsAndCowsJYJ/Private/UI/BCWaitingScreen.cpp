//BCWaitingScreen.cpp


#include "UI/BCWaitingScreen.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Game/BCGameStateBase.h"
#include "Player/BCPlayerController.h"

void UBCWaitingScreen::NativeConstruct()
{
	//부모의 위젯 구성 처리를 먼저 실행한 뒤, BindWidget으로 연결된 버튼에 클릭 함수를 등록함
	Super::NativeConstruct();
	//같은 위젯이 다시 구성되어도 클릭 한 번에 이동 요청이 여러 번 실행되지 않도록 중복 등록을 막는다
	Button_MainMenu->OnClicked.AddUniqueDynamic(this, &ThisClass::OnMainMenuButtonClicked);
}

void UBCWaitingScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//MatchState는 서버(GameMode)가 GameState에 넣어 복제해 준 값이다. 위젯은 읽어서 보여 주기만 함
	ABCGameStateBase* BCGS = GetWorld()->GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == false)
	{
		return;
	}

	//게임 중이면 판을 숨기고(Collapsed: 안 보이고 자리도 차지하지 않음), 대기·매치 중이면 보임
	//빈 영역을 눌러도 아래 태블릿 채팅창을 쓸 수 있어야 함
	//SelfHitTestInvisible: 판 자체는 클릭을 통과시키고 안쪽 버튼은 누를 수 있음
	//판 자체는 클릭을 받지 않고 자식 버튼만 클릭할 수 있음. 빈 영역의 클릭은 아래 채팅창으로 계속 통과함
	Panel_Waiting->SetVisibility(BCGS->MatchState == EBCMatchState::Playing ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);

	//2명이 모인 직후(Matched)에만 "매치되었습니다!", 그 외(Waiting)는 기다리는 중
	TextBlock_WaitingMessage->SetText(FText::FromString(BCGS->MatchState == EBCMatchState::Matched ? TEXT("매치되었습니다!") : TEXT("상대를 기다리는 중...")));
}

void UBCWaitingScreen::OnMainMenuButtonClicked()
{
	//CreateWidget에 주인으로 넘겨졌던 게임 PC를 얻는다. 다른 플레이어의 PC나 서버 전체를 이동시키지 않는다
	ABCPlayerController* PlayerController = Cast<ABCPlayerController>(GetOwningPlayer());
	if (IsValid(PlayerController) == true)
	{
		//위젯은 입력만 전달하고, 이동 주소와 ClientTravel 호출은 PC의 함수 한 곳에서 관리함
		PlayerController->ReturnToMainMenu();
	}
}
