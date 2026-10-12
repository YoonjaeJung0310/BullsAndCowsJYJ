//BCTurnTimer.cpp


#include "UI/BCTurnTimer.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Game/BCGameStateBase.h"
#include "Audio/BCAudioSubsystem.h"
#include "Player/BCPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Player/BCPlayerController.h"

void UBCTurnTimer::NativeConstruct()
{
	Super::NativeConstruct();
	DefaultTimerColor = TextBlock_TurnTimer->GetColorAndOpacity();
	//재구성돼도 한 번만 연결함. GameState를 읽기 전에는 숨겨 두고 게임 중일 때만 보여 줌
	Button_MainMenu->OnClicked.AddUniqueDynamic(this, &ThisClass::OnMainMenuButtonClicked);
	Button_MainMenu->SetVisibility(ESlateVisibility::Collapsed);
}

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
	//대기 화면에는 이미 가운데 메뉴 복귀 버튼이 있으므로, 전광판 아래 버튼은 게임 중에만 표시함
	Button_MainMenu->SetVisibility(BCGS->MatchState == EBCMatchState::Playing ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	//0이면 턴이 없는 상태(결과 공지 중)라서 글자를 비움
	if (BCGS->RemainingTurnTime > 0)
	{
		//누구 차례인지 표시는 모두 보지만, 턴 시작·마감 소리는 지금 입력해야 하는 사람에게만 재생함
		const APlayerController* PlayerController = GetOwningPlayer();
		const ABCPlayerState* PlayerState = IsValid(PlayerController) ? PlayerController->GetPlayerState<ABCPlayerState>() : nullptr;
		const bool bIsMyTurn = IsValid(PlayerState) && PlayerState->PlayerNameString == BCGS->CurrentTurnPlayerNameString;
		if (bIsMyTurn && (PreviousTurnPlayerName != BCGS->CurrentTurnPlayerNameString || PreviousRemainingTime == 0))
		{
			UBCAudioSubsystem::PlayEffect(this, TEXT("S_Turn"));
		}
		if (bIsMyTurn && (PreviousRemainingTime != BCGS->RemainingTurnTime || PreviousTurnPlayerName != BCGS->CurrentTurnPlayerNameString) && BCGS->RemainingTurnTime <= 5)
		{
			//5초 구간으로 들어갈 때 경고 한 번, 마지막 3초는 초가 바뀔 때마다 짧게 알림. 매 프레임 울리지 않음
			if (PreviousRemainingTime > 5 || PreviousRemainingTime == 0 || PreviousTurnPlayerName != BCGS->CurrentTurnPlayerNameString)
			{
				UBCAudioSubsystem::PlayEffect(this, TEXT("S_Warning"));
			}
			else if (BCGS->RemainingTurnTime <= 3)
			{
				UBCAudioSubsystem::PlayEffect(this, TEXT("S_Tick"));
			}
		}
		//경고 구간에서는 판정 글씨와 같은 빨간 계열로 표시함. 새 턴은 WBP의 원래 색을 다시 씀
		TextBlock_TurnTimer->SetColorAndOpacity(BCGS->RemainingTurnTime <= 5 ? FSlateColor(FLinearColor(1.0f, 0.12f, 0.1f, 1.0f)) : DefaultTimerColor);
		//게임 중에는 기회 줄도 보여 줌. 대기 중에 접었던 줄을 다시 켜야 두 줄이 같이 나옴
		TextBlock_RemainingChance->SetVisibility(ESlateVisibility::HitTestInvisible);
		//%s 자리에 FString을 넣을 때는 앞에 *를 붙여 TCHAR* 문자열로 바꿔 넘김. %d는 정수
		//TextBlock의 SetText는 FText를 받으니 FText::FromString으로 바꿔 넣음
		//왼쪽 전광판은 차례·기회, 오른쪽은 남은 시간으로 보여 줌
		TextBlock_TurnPlayer->SetText(FText::FromString(FString::Printf(TEXT("%s 차례"), *BCGS->CurrentTurnPlayerNameString)));
		TextBlock_RemainingChance->SetText(FText::FromString(FString::Printf(TEXT("기회 %d번 남음"), BCGS->CurrentTurnRemainingGuessCount)));
		TextBlock_TurnTimer->SetText(FText::FromString(FString::Printf(TEXT("%d초"), BCGS->RemainingTurnTime)));
		//내 이름의 복제가 늦어도 처음 내 차례임을 알게 됐을 때 시작음을 낼 수 있게, 남의 차례는 빈 이름으로 기억함
		PreviousTurnPlayerName = bIsMyTurn ? BCGS->CurrentTurnPlayerNameString : FString();
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
		TextBlock_TurnTimer->SetColorAndOpacity(DefaultTimerColor);
		PreviousTurnPlayerName.Empty();
	}
	//표시를 마친 값만 기억함. 복제된 시간이 바뀔 때 다음 Tick에서 한 번 더 검사함
	PreviousRemainingTime = BCGS->RemainingTurnTime;
}

void UBCTurnTimer::OnMainMenuButtonClicked()
{
	if (ABCPlayerController* PlayerController = Cast<ABCPlayerController>(GetOwningPlayer()))
	{
		//대기 화면과 같은 복귀 함수를 재사용함. 이 함수 안에서 클릭음과 내 클라이언트의 메뉴 이동을 처리함
		PlayerController->ReturnToMainMenu();
	}
}
