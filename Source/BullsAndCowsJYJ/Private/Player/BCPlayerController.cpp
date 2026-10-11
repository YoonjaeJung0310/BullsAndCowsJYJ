//BCPlayerController.cpp


#include "Player/BCPlayerController.h"
#include "UI/BCChatInput.h"
#include "UI/BCTurnTimer.h"
#include "UI/BCScoreBoard.h"
#include "UI/BCWaitingScreen.h"
#include "Kismet/GameplayStatics.h"
#include "Game/BCGameModeBase.h"
#include "Net/UnrealNetwork.h"


ABCPlayerController::ABCPlayerController()
{
	//이 액터를 네트워크 복제 대상으로 켠다 → NotificationText 같은 Replicated 변수와 RPC가 네트워크를 탈 수 있음
	//참고: 플레이어용 PC는 엔진이 스폰할 때(UWorld::SpawnPlayActor)도 SetReplicates(true)를 해 준다
	bReplicates = true;
}

void ABCPlayerController::ReturnToMainMenu()
{
	//서버에 있는 다른 플레이어의 PC에서 잘못 호출되어도 이동을 시작하지 않도록 로컬 PC만 처리함
	if (IsLocalController() == false)
	{
		return;
	}

	//서버 주소가 아닌 프로젝트 안의 메뉴 맵으로 이동함. 기존 연결은 종료되고 메뉴용 GameMode와 PC가 생성됨
	//TRAVEL_Absolute는 이전 서버 주소와 BCName 같은 URL 옵션을 이어 붙이지 않으므로 다시 혼자 실행하는 메뉴로 돌아감
	ClientTravel(TEXT("/Game/BullsAndCowsJYJ/Maps/MainMenu"), ETravelType::TRAVEL_Absolute);
}

//템플릿 함수는 쓰는 곳(BeginPlay)보다 위에 정의해 둔다. 이 .cpp 안에서만 쓰니 헤더가 아니라 여기에 둬도 됨
template <typename WidgetT>
WidgetT* ABCPlayerController::CreateWidgetOnViewport(TSubclassOf<WidgetT> InWidgetClass, int32 InZOrder)
{
	//블루프린트에서 위젯 클래스를 지정해 뒀을 때만 만듦 (비어 있으면 CreateWidget에 넘길 클래스가 없음)
	if (IsValid(InWidgetClass) == false)
	{
		return nullptr;
	}

	//CreateWidget<T>(주인, 클래스): 위젯 객체를 만듦. 첫 인자 this(이 PC)가 위젯의 주인이 됨 → 위젯에서 GetOwningPlayer()로 얻는 값
	WidgetT* NewWidget = CreateWidget<WidgetT>(this, InWidgetClass);
	if (IsValid(NewWidget) == true)
	{
		//AddToViewport: 만든 위젯을 화면에 붙임. 만들기만 하고 이걸 안 부르면 화면에 안 보임
		NewWidget->AddToViewport(InZOrder);
	}
	return NewWidget;
}

void ABCPlayerController::BeginPlay()
{
	//부모(APlayerController)의 BeginPlay를 먼저 실행함 (블루프린트의 Event BeginPlay도 여기서 불림)
	Super::BeginPlay();

	//IsLocalController(): 이 컴퓨터에서 직접 조종하는 PC인가?
	//클라이언트의 자기 PC, 리슨 서버 호스트의 자기 PC → true
	//서버에 있는 다른 플레이어의 PC → false (서버 쪽이 원본(Authority)이지만, 조종은 그 플레이어가 자기 컴퓨터에서 함)
	if (IsLocalController() == false)
	{
		return;
	}

	//입력 모드를 UI 전용으로 바꿈. 키보드·마우스 입력이 게임(폰 조작)이 아니라 위젯(채팅창)으로만 감
	FInputModeUIOnly InputModeUIOnly;
	SetInputMode(InputModeUIOnly);
	//메뉴에서 넘어오면 메뉴용 PC가 사라지고 이 PC가 새로 생기므로, 채팅창을 클릭할 수 있게 마우스 커서를 다시 켠다
	bShowMouseCursor = true;

	//위젯은 내 화면에만 띄우면 되니 위의 IsLocalController 검사를 지난 뒤에 만듦 (서버에 있는 남의 PC는 여기까지 안 옴)
	//ZOrder 순서: 배경 그림이 들어 있는 전광판(0)을 맨 아래 깔고 → 턴 정보(1) → 태블릿 채팅창(2) → 대기 화면(3) → 공지(4)를 맨 위에 그린다
	ScoreBoardWidgetInstance = CreateWidgetOnViewport(ScoreBoardWidgetClass, 0);
	TurnTimerWidgetInstance = CreateWidgetOnViewport(TurnTimerWidgetClass, 1);
	ChatInputWidgetInstance = CreateWidgetOnViewport(ChatInputWidgetClass, 2);
	WaitingScreenWidgetInstance = CreateWidgetOnViewport(WaitingScreenWidgetClass, 3);
	NotificationTextWidgetInstance = CreateWidgetOnViewport(NotificationTextWidgetClass, 4);
}

//BCChatInput 위젯 → (여기) → ServerRPC → 서버의 GameMode::PrintChatMessageString 순서로 채팅이 올라감
void ABCPlayerController::SetChatMessageString(const FString& InChatMessageString)
{
	//내 PC일 때만 서버로 보낸다. Server RPC는 주인 클라이언트(내 PC)에서 불러야 서버로 전달됨
	if (IsLocalController() == true)
	{
		//이름과 [쓴 기회/최대 기회]는 서버에서 붙여 줌
		//서버가 입력 전체를 검사해야 하므로 이름을 붙이지 않고 보냄
		//- 판정으로 횟수가 늘어난 "뒤에" 붙여야 이번 입력까지 센 횟수가 보임
		ServerRPCPrintChatMessageString(InChatMessageString);
	}

}

void ABCPlayerController::PrintChatMessageString(const FString& InChatMessageString)
{
	//받은 문자열을 태블릿 채팅창에 한 줄 추가해 줌
	//위젯은 내 PC(로컬)에서만 만들어지므로, 아직 위젯이 없을 때(접속 직후 등) 온 줄은 IsValid에서 걸러진다
	if (IsValid(ChatInputWidgetInstance) == true)
	{
		ChatInputWidgetInstance->AddChatMessage(InChatMessageString);
	}
}

void ABCPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	//부모 APlayerController의 복제 변수 등록을 먼저 하고, 우리 변수(NotificationText)를 추가함
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, NotificationText);
}

//서버에서 실행됨. 채팅을 직접 처리하지 않고 서버에 있는 GameMode에게 넘김
void ABCPlayerController::ServerRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	//GetGameMode: 서버에서는 GameMode를 돌려주고, 클라이언트에서 부르면 nullptr이다 (GameMode는 서버에만 있음)
	//이 함수는 Server RPC라서 서버에서 돌고 있으니 GameMode를 얻을 수 있음
	AGameModeBase* GM = UGameplayStatics::GetGameMode(this);
	if (IsValid(GM) == true)
	{
		ABCGameModeBase* BCGM = Cast<ABCGameModeBase>(GM);
		if (IsValid(BCGM) == true)
		{
			//this = 이 채팅을 보낸 플레이어의 PC (서버에 있는 그 플레이어 PC). GameMode는 이걸로 누가 보냈는지 안다
			BCGM->PrintChatMessageString(this, InChatMessageString);
		}
	}
}

//클라이언트(이 PC의 주인)에서 실행됨. 서버가 보낸 문자열을 내 화면에 출력함
void ABCPlayerController::ClientRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	PrintChatMessageString(InChatMessageString);
}
