//BCMainMenu.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BCMainMenu.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

//메인 메뉴 위젯의 C++ 부모 클래스 (WBP_MainMenu가 상속). MainMenu 레벨에서 ABCMenuPlayerController가 만듦
//이름 입력칸 + 게임 시작 / 설명 / 나가기 버튼 + 설명 판(규칙 요약)
UCLASS()
class BULLSANDCOWSJYJ_API UBCMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	//BindWidget: WBP 안에 이름과 타입이 똑같은 위젯이 있어야 함. 없으면 WBP 컴파일 에러
	//게임에서 쓸 이름을 입력하는 칸
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_PlayerName;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Start;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Help;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Quit;

	//이름이 잘못됐을 때 이유나 "서버에 접속하는 중..."을 띄우는 글자
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_MenuMessage;

	//설명 버튼으로 켜고 끄는 판. 안에 TextBlock_HelpText를 넣음
	//UWidget으로 받으므로 Border, VerticalBox 등 어떤 위젯이든 이름만 맞으면 연결됨
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> Panel_Help;

	//규칙 요약 글자. 내용은 C++(NativeConstruct)에서 넣음
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_HelpText;

	//설명 판 안의 "닫기" 버튼. 설명 판이 메뉴 버튼들을 덮어서 [설명]을 다시 눌러 닫기 어려워 판 안에 둔다
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_HelpClose;

	//버튼 클릭 이벤트 연결, 설명 판 숨기기, 규칙 글 넣기
	virtual void NativeConstruct() override;

protected:
	//버튼의 OnClicked에 연결할 함수들. AddUniqueDynamic(다이내믹 델리게이트)은 함수를 이름으로 찾기 때문에 UFUNCTION이 필요하다
	//OnClicked 델리게이트는 매개변수가 없으므로 함수도 매개변수 없이 만듦

	//이름을 검사하고, 괜찮으면 메뉴용 PC에게 게임 서버 접속을 맡긴다
	UFUNCTION()
	void OnStartButtonClicked();

	//설명 판을 켜고 끈다 ([설명] 버튼과 설명 판 안의 [닫기] 버튼이 같이 씀)
	UFUNCTION()
	void OnHelpButtonClicked();

	//게임을 끈다 (PIE에서는 플레이가 끝난다)
	UFUNCTION()
	void OnQuitButtonClicked();
};
