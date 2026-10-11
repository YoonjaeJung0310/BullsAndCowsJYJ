//BCPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Game/BCGameModeBase.h"
#include "BCPlayerController.generated.h"

//전방 선언: 이 헤더에서는 위젯 클래스들을 포인터와 TSubclassOf로만 쓰니 #include 없이 이름만 알려도 됨
class UBCChatInput;
class UBCTurnTimer;
class UBCScoreBoard;
class UBCWaitingScreen;
class UUserWidget;

//PlayerController: 플레이어 한 명의 입력과 UI를 맡는다
//서버에는 접속한 모든 플레이어의 PC가 있고, 각 클라이언트에는 자기 PC 하나만 있음 (PC는 주인 클라이언트에만 복제됨)
UCLASS()
class BULLSANDCOWSJYJ_API ABCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	//생성자: 변수의 기본값을 정함 (여기서는 복제 켜기)
	ABCPlayerController();

	//PC가 월드에서 시작될 때 불림. 내 PC일 때만 입력 모드를 바꾸고 위젯을 만듦
	virtual void BeginPlay() override;

	//채팅 위젯(BCChatInput)에서 엔터를 치면 불림. 입력한 글을 Server RPC로 서버에 올려 보낸다
	void SetChatMessageString(const FString& InChatMessageString);

	//받은 문자열을 태블릿 채팅창(ChatInputWidgetInstance)에 한 줄 추가함
	void PrintChatMessageString(const FString& InChatMessageString);

	//대기 화면의 메인 메뉴 버튼이 호출함. 이 PC를 조종하는 로컬 플레이어만 서버를 떠나 MainMenu로 이동함
	//UI 위젯은 이동 요청만 보내고, 실제 레벨 이동은 입력과 UI를 맡는 PC에서 처리함
	void ReturnToMainMenu();

	//Client RPC: 서버에서 부르면 이 PC의 주인 클라이언트에서 _Implementation이 실행됨 (서버 → 특정 클라이언트 한 명)
	UFUNCTION(Client, Reliable)
	void ClientRPCPrintChatMessageString(const FString& InChatMessageString);

	//Server RPC: 주인 클라이언트에서 부르면 서버에서 _Implementation이 실행됨 (클라이언트 → 서버)
	//복제는 서버 → 클라이언트 한 방향뿐이라 클라이언트가 서버에 무언가를 알릴 때는 이렇게 RPC를 씀
	//클라이언트가 "소유한" 액터(자기 PC, 자기 폰 등)에서만 부를 수 있음
	UFUNCTION(Server, Reliable)
	void ServerRPCPrintChatMessageString(const FString& InChatMessageString);

	//공지 문구. Replicated: 서버에서 바꾸면 이 PC의 주인 클라이언트로 복제됨
	//BlueprintReadOnly: 블루프린트에서 읽기만 할 수 있게 공개함 → 공지 위젯의 바인딩 함수가 이 값을 읽음
	UPROPERTY(Replicated, BlueprintReadOnly)
	FText NotificationText;

	//NotificationText를 복제 목록에 등록함
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	//만들 채팅 위젯의 클래스(WBP)
	//EditDefaultsOnly: 블루프린트 클래스 기본값(Class Defaults)에서만 고를 수 있음. 월드에 놓인 개별 인스턴스에서는 못 바꿈
	//TSubclassOf<UBCChatInput>: UBCChatInput과 그 자식 클래스만 고를 수 있게 제한함
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBCChatInput> ChatInputWidgetClass;

	//CreateWidget으로 만든 실제 채팅 위젯. UPROPERTY()가 있어야 GC가 이 포인터를 참조로 인식함
	UPROPERTY()
	TObjectPtr<UBCChatInput> ChatInputWidgetInstance;

	//만들 공지 위젯의 클래스. 공지 위젯은 이 프로젝트에 전용 C++ 클래스가 없음
	//TSubclassOf<UUserWidget>이라 모든 위젯 블루프린트(WBP)를 고를 수 있음
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> NotificationTextWidgetClass;

	//CreateWidget으로 만든 실제 공지 위젯
	UPROPERTY()
	TObjectPtr<UUserWidget> NotificationTextWidgetInstance;

	//누구 차례인지 + 남은 시간을 보여 주는 위젯. BP_BCPlayerController에서 WBP_TurnTimer를 넣어 준다
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBCTurnTimer> TurnTimerWidgetClass;

	UPROPERTY()
	TObjectPtr<UBCTurnTimer> TurnTimerWidgetInstance;

	//가운데 전광판(마지막 숫자 + 판정 결과)과 야구장 배경 그림을 담은 위젯. BP_BCPlayerController에서 WBP_ScoreBoard를 넣어 준다
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBCScoreBoard> ScoreBoardWidgetClass;

	UPROPERTY()
	TObjectPtr<UBCScoreBoard> ScoreBoardWidgetInstance;

	//"상대를 기다리는 중..." / "매치되었습니다!"를 띄우는 대기 화면 위젯. BP_BCPlayerController에서 WBP_WaitingScreen을 넣어 준다
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBCWaitingScreen> WaitingScreenWidgetClass;

	UPROPERTY()
	TObjectPtr<UBCWaitingScreen> WaitingScreenWidgetInstance;

	//위젯 클래스가 지정돼 있으면 위젯을 만들어 화면에 붙이고 돌려줌. 클래스가 비어 있으면 nullptr
	//위젯 클래스 확인 → 객체 생성 → 화면 추가. 다섯 위젯이 이 순서로 만들어짐
	//template <typename WidgetT>: 위젯 타입을 부르는 쪽에서 정함 → UBCChatInput*, UBCTurnTimer*처럼 원래 타입 그대로 돌려받아 Cast가 필요 없음
	//InZOrder: 화면에 겹쳐 그리는 순서. 클수록 위에 그려진다
	template <typename WidgetT>
	WidgetT* CreateWidgetOnViewport(TSubclassOf<WidgetT> InWidgetClass, int32 InZOrder);

};
