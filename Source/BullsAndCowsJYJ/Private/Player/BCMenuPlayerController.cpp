//BCMenuPlayerController.cpp


#include "Player/BCMenuPlayerController.h"

#include "UI/BCMainMenu.h"

void ABCMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//내 컴퓨터에서 조종하는 PC일 때만 위젯을 만듦 (메뉴 레벨은 혼자 실행되지만 게임 PC와 같은 규칙으로 맞춘다)
	if (IsLocalController() == false)
	{
		return;
	}

	//메뉴는 마우스로 버튼을 눌러야 하니 커서를 보이게 하고, 입력을 UI로만 보낸다
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());

	if (IsValid(MainMenuWidgetClass) == true)
	{
		MainMenuWidgetInstance = CreateWidget<UBCMainMenu>(this, MainMenuWidgetClass);
		if (IsValid(MainMenuWidgetInstance) == true)
		{
			MainMenuWidgetInstance->AddToViewport();
		}
	}
}

void ABCMenuPlayerController::JoinGameServer(const FString& InPlayerNameString)
{
	//ClientTravel(주소, TRAVEL_Absolute): 지금 레벨을 떠나 그 주소의 서버로 접속함
	//접속에 성공하면 서버의 게임 레벨(Chatting)이 열리고, 이 메뉴용 PC 대신 게임 레벨의 PC(BP_BCPlayerController)가 새로 생긴다
	//접속에 실패하면: 패키징한 게임·별도 프로세스 창은 기본 레벨(GameDefaultMap = MainMenu)로 돌아가고, 에디터 안 PIE 창은 메뉴에 그대로 남는다
	//이름 옵션 키를 엔진의 ?Name= 이 아니라 ?BCName= 으로 쓰는 이유: 접속 과정에서 엔진이 ?Name= 값을 온라인 서브시스템 닉네임으로 덮어씀 (ABCGameModeBase::InitNewPlayer 주석 참고)
	ClientTravel(FString::Printf(TEXT("%s?BCName=%s"), *ServerAddress, *InPlayerNameString), ETravelType::TRAVEL_Absolute);
}
