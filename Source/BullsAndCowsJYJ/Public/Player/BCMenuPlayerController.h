//BCMenuPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BCMenuPlayerController.generated.h"

class UBCMainMenu;

//메인 메뉴 레벨(MainMenu)에서 쓰는 PlayerController. 메뉴 위젯을 띄우고, 게임 시작을 누르면 게임 서버로 접속함
//게임 레벨의 ABCPlayerController(채팅 RPC·공지·게임 위젯 담당)와 하는 일이 달라서 따로 만듦
UCLASS()
class BULLSANDCOWSJYJ_API ABCMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	//내 PC일 때 마우스 커서를 켜고 메뉴 위젯을 화면에 붙임
	virtual void BeginPlay() override;

	//게임 서버로 접속함. 이름은 주소 뒤에 ?BCName=이름 으로 붙여 보내고, 서버의 ABCGameModeBase::InitNewPlayer가 꺼내 PlayerState에 넣음
	void JoinGameServer(const FString& InPlayerNameString);

protected:
	//만들 메뉴 위젯 클래스. BP_BCMenuPlayerController의 클래스 기본값에서 WBP_MainMenu를 넣어 준다
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBCMainMenu> MainMenuWidgetClass;

	//CreateWidget으로 만든 실제 메뉴 위젯. UPROPERTY()가 있어야 GC가 이 포인터를 참조로 인식함
	UPROPERTY()
	TObjectPtr<UBCMainMenu> MainMenuWidgetInstance;

	//접속할 서버 주소(IP:포트). PIE에서 서버를 따로 띄우면 기본 포트가 17777이다 (에디터 설정 Play > Multiplayer Options > Server Port)
	//패키징한 데디케이티드 서버는 기본 포트가 7777이라, 블루프린트 클래스 기본값에서 바꿀 수 있게 EditDefaultsOnly로 둔다
	UPROPERTY(EditDefaultsOnly)
	FString ServerAddress = TEXT("127.0.0.1:17777");
};
