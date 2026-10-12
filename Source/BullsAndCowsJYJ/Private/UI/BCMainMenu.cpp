//BCMainMenu.cpp


#include "UI/BCMainMenu.h"
#include "Audio/BCAudioSubsystem.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h" //UKismetSystemLibrary::QuitGame
#include "Player/BCMenuPlayerController.h"
#include "Player/BCPlayerState.h" //ABCPlayerState::MaxPlayerNameLength (서버와 같은 이름 글자 수 제한을 쓰려고)

void UBCMainMenu::NativeConstruct()
{
	Super::NativeConstruct();

	//AddUniqueDynamic: 이미 같은 함수가 연결돼 있으면 또 연결하지 않는다
	//채팅창(UBCChatInput)의 "IsAlreadyBound로 확인 → AddDynamic"을 한 줄로 쓴 것과 같다
	Button_Start->OnClicked.AddUniqueDynamic(this, &ThisClass::OnStartButtonClicked);
	Button_Help->OnClicked.AddUniqueDynamic(this, &ThisClass::OnHelpButtonClicked);
	Button_Quit->OnClicked.AddUniqueDynamic(this, &ThisClass::OnQuitButtonClicked);
	//[닫기]는 설명 판이 보일 때만 눌리므로, 켜고 끄는 함수(OnHelpButtonClicked)를 그대로 쓰면 항상 닫힌다 → 함수를 새로 만들지 않는다
	Button_HelpClose->OnClicked.AddUniqueDynamic(this, &ThisClass::OnHelpButtonClicked);

	//설명 판은 처음엔 숨겨 둔다. Collapsed: 안 보이고 자리도 차지하지 않는다
	Panel_Help->SetVisibility(ESlateVisibility::Collapsed);
	TextBlock_MenuMessage->SetText(FText::GetEmpty());

	//설명 판에 보여 줄 규칙. 아래 숫자는 게임에 쓰는 값과 같아야 함
	//(기회 3번 = ABCPlayerState 생성자의 MaxGuessCount, 턴 30초 = ABCGameModeBase::TurnTimeLimit, 2명부터 시작 = ABCGameModeBase::MinPlayerCountToStart)
	//그 값을 바꾸면 이 글도 같이 고쳐야 함
	//TEXT("...") TEXT("...")처럼 문자열을 나란히 쓰면 컴파일러가 하나로 이어 붙임 → 긴 글을 줄마다 나눠 쓸 수 있음 (\n은 줄바꿈)
	TextBlock_HelpText->SetText(FText::FromString(
		TEXT("[숫자 야구 규칙]\n")
		TEXT("1. 서버가 1~9 중 서로 다른 숫자 3개로 정답을 만듭니다. (예: 386)\n")
		TEXT("2. 자기 차례에 숫자 3개를 입력합니다.\n")
		TEXT("   S(스트라이크): 숫자와 자리가 모두 맞음\n")
		TEXT("   B(볼): 숫자는 있지만 자리가 다름\n")
		TEXT("   OUT: 맞는 숫자가 하나도 없음\n")
		TEXT("3. 한 사람당 기회는 3번. 3S를 먼저 맞히면 승리!\n")
		TEXT("4. 한 턴은 30초. 시간 안에 입력하지 않으면 기회가 1번 줄어듭니다.\n")
		TEXT("5. 3자리가 아니거나 중복·문자·0이 들어간 입력은 기회가 줄지 않으니 다시 입력하면 됩니다.\n")
		TEXT("6. 모두 기회를 다 쓰면 무승부, 게임 중 상대가 나가면 남은 사람이 승리합니다.\n")
		TEXT("7. 2명이 모이면 게임이 시작됩니다.")));
}

void UBCMainMenu::OnStartButtonClicked()
{
	//메뉴 버튼은 같은 짧은 클릭음을 재사용함. 잘못된 이름으로 접속이 막혀도 버튼을 누른 소리는 들림
	UBCAudioSubsystem::PlayEffect(this, TEXT("S_Click"));
	//TrimStartAndEnd: 앞뒤 공백을 뺀 새 문자열
	FString PlayerNameString = EditableTextBox_PlayerName->GetText().ToString().TrimStartAndEnd();
	if (PlayerNameString.IsEmpty() == true || PlayerNameString.Len() > ABCPlayerState::MaxPlayerNameLength)
	{
		TextBlock_MenuMessage->SetText(FText::FromString(FString::Printf(TEXT("이름은 1~%d글자로 입력해 주세요."), ABCPlayerState::MaxPlayerNameLength)));
		return;
	}

	//이름은 접속 주소 뒤에 "?BCName=이름"으로 붙어 감. 주소는 ?로 옵션을, =로 이름과 값을, #으로 다른 부분을 나누므로
	//이름에 ? = # 이나 띄어쓰기가 있으면 주소가 엉뚱하게 잘린다 → 보내기 전에 막는다
	for (TCHAR C : PlayerNameString)
	{
		if (FChar::IsWhitespace(C) == true || C == TEXT('?') || C == TEXT('=') || C == TEXT('#'))
		{
			TextBlock_MenuMessage->SetText(FText::FromString(TEXT("이름에 띄어쓰기와 ? = # 는 쓸 수 없어요.")));
			return;
		}
	}

	//접속은 위젯이 아니라 PC가 맡는다 (위젯 = 버튼·입력 받기, PC = 실제 이동)
	ABCMenuPlayerController* MenuPlayerController = Cast<ABCMenuPlayerController>(GetOwningPlayer());
	if (IsValid(MenuPlayerController) == true)
	{
		TextBlock_MenuMessage->SetText(FText::FromString(TEXT("서버에 접속하는 중...")));
		MenuPlayerController->JoinGameServer(PlayerNameString);
	}
}

void UBCMainMenu::OnHelpButtonClicked()
{
	UBCAudioSubsystem::PlayEffect(this, TEXT("S_Click"));
	//IsVisible(): 지금 보이는 상태인가? → 보이면 숨기고, 숨겨져 있으면 보임 (같은 버튼으로 열고 닫기)
	//SelfHitTestInvisible: 판 자체는 클릭을 받지 않는다 → 설명 판이 버튼 위를 덮어도 아래 버튼을 다시 눌러 닫을 수 있음
	Panel_Help->SetVisibility(Panel_Help->IsVisible() == true ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void UBCMainMenu::OnQuitButtonClicked()
{
	UBCAudioSubsystem::PlayEffect(this, TEXT("S_Click"));
	//QuitGame(월드 기준 객체, 끌 플레이어, 종료 방식, 플랫폼 제한 무시 여부)
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
