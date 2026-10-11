//BCGameModeBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BCGameModeBase.generated.h" //UHT가 만든 리플렉션 코드. 항상 include 목록의 맨 마지막에 와야 함

//전방 선언: "ABCPlayerController라는 클래스가 있다"는 것만 알린다. 이 헤더에서는 포인터로만 쓰니 전체 정의(#include)가 필요 없음
//BCPlayerController.h가 이 헤더를 include하고 있어서, 여기서 거꾸로 BCPlayerController.h를 include하면 서로를 include하는 순환이 생긴다
class ABCPlayerController;
//enum class도 밑바탕 타입(uint8)을 적으면 전방 선언할 수 있음. 정의는 BCGameStateBase.h에 있음
enum class EBCMatchState : uint8;

//GameMode: 게임 규칙(정답 생성, 판정, 턴 진행)을 담당함. 서버에만 존재하고 클라이언트로 복제되지 않는다
//게임 월드의 규칙은 서버가 처리함. 클라는 자기 PC의 Server RPC로 입력을 전달
//UCLASS(): 언리얼 리플렉션 시스템에 이 클래스를 등록함 (블루프린트 상속, GC, 복제 같은 엔진 기능의 전제 조건)
//BULLSANDCOWSJYJ_API: 이 클래스를 모듈 밖으로 내보내는(export) 매크로. 다른 모듈에서도 이 클래스를 쓸 수 있게 해 준다
UCLASS()
class BULLSANDCOWSJYJ_API ABCGameModeBase : public AGameModeBase
{
	//UHT가 만든 코드가 여기에 펼쳐진다. Super(부모 클래스), ThisClass(이 클래스) 같은 별명(typedef)도 여기서 생긴다
	GENERATED_BODY()

public:
	//플레이어가 서버 접속(로그인)을 마친 직후 서버에서 불림. 엔진의 PostLogin이 이 함수를 부름
	//순서: PostLogin → OnPostLogin(여기) → HandleStartingNewPlayer(폰 스폰) → 그래서 여기서는 아직 플레이어의 폰이 없음
	//virtual + override: 부모(AGameModeBase)의 가상 함수를 덮어씀. override를 붙이면 부모에 같은 모양의 함수가 없을 때 컴파일 에러로 알려 줌
	virtual void OnPostLogin(AController* NewPlayer) override;

	//로그인 도중(OnPostLogin보다 먼저) 서버에서 불림. Options = 접속 주소 뒤에 붙은 ?옵션들
	//메뉴에서 보낸 ?BCName=이름 을 꺼내 겹치지 않게 정리한 뒤 PlayerState의 PlayerNameString에 넣음
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

	//플레이어가 접속을 끊으면 서버에서 불림 (PC의 Destroyed() 안이라 아직 PC와 PlayerState를 쓸 수 있음)
	//게임 중에 한 명만 남으면 그 사람이 승리, 여럿 남았는데 나간 사람이 차례였으면 기다리지 않고 바로 다음 턴
	virtual void Logout(AController* Exiting) override;

	//1~9 중 서로 다른 숫자 3개를 무작위로 뽑아 정답 문자열(예: "472")을 만듦
	FString GenerateSecretNumber();

	//규칙에 맞으면 true. 틀리면 false와 함께 OutInvalidReasonString에 "왜 틀렸는지" 안내 문구를 담아 줌 (12 → 3자리, 112 → 중복 등)
	bool IsGuessNumberString(const FString& InNumberString, FString& OutInvalidReasonString);

	//입력을 정답(멤버 SecretNumberString)과 자리별로 비교해 "1S1B" 같은 결과 문자열을 만듦. 하나도 안 맞으면 "OUT"
	//입력만 받음. 정답은 GameMode가 이미 갖고 있어서 따로 넘기지 않는다
	//const FString&: 문자열을 복사하지 않고(참조) 읽기만 함(const)
	FString CheckAnswer(const FString& InGuessNumberString);

	//게임이 시작될 때 한 번 불림. 첫 정답을 여기서 만듦
	virtual void BeginPlay() override;

	//플레이어가 보낸 채팅을 처리하는 입구. 클라이언트의 Server RPC(ServerRPCPrintChatMessageString)가 서버에서 이 함수를 부름
	//내 차례면 숫자 야구 입력으로 판정하고, 아니면 일반 채팅으로 모두에게 뿌린다
	void PrintChatMessageString(ABCPlayerController* InChattingPlayerController, const FString& InChatMessageString);

	//이 플레이어의 시도 횟수(PlayerState의 CurrentGuessCount)를 1 올린다
	void IncreaseGuessCount(ABCPlayerController* InChattingPlayerController);

	//새 정답을 만들고, 모든 플레이어의 시도 횟수와 공지를 지운 뒤, 첫 번째 플레이어 턴부터 다시 시작함
	//2명 이상이면 Playing으로 바꾸고 시작, 모자라면 Waiting(대기 화면)으로 돌린다
	//불리는 때: 매치 직후("매치되었습니다!" MatchedNoticeTime초 뒤), 승리·무승부 공지 ResetGameDelay초 뒤
	void ResetGame();

	//승리나 무승부로 게임이 끝났으면 true. 호출한 쪽은 false일 때만 다음 턴으로 넘김
	bool JudgeGame(ABCPlayerController* InChattingPlayerController, int InStrikeCount);

protected:
	//월드의 PC를 돌면서 각 플레이어에게 같은 채팅 한 줄을 보내 줌
	void BroadcastChatMessageString(const FString& InChatMessageString);

	//모든 플레이어의 공지 위젯 문구(NotificationText)를 같은 값으로 바꿈. EndGame(결과 공지)과 ClearConnectedNotification(지우기)이 같이 씀
	void BroadcastNotificationText(const FText& InNotificationText);

	//접속 공지("Connected to the game server.")를 지움. OnPostLogin이 건 타이머가 ConnectedNotificationTime초 뒤에 부름
	void ClearConnectedNotification();

	//접속 중인 플레이어가 MinPlayerCountToStart명 이상이라 새 판을 시작해도 되는가? (OnPostLogin, ResetGame에서 확인)
	bool CanStartGame() const;

	//AllPlayerControllers 중 아직 접속해 있는(IsValid) PC만 모아 돌려줌. 나간 사람 자리는 빠진다
	//CanStartGame(인원 수)과 Logout(남은 한 명 찾기)이 같이 씀
	TArray<ABCPlayerController*> GetValidPlayerControllers() const;

	//메뉴에서 입력한 이름을 정리한다: 앞뒤 공백 제거 → 최대 글자 수로 자르기 → 비었으면 "Player" → 다른 사람과 겹치면 뒤에 2, 3…을 붙임
	FString MakeUniquePlayerName(const FString& InNameString) const;

	//GameState의 MatchState를 바꿈. 대기 화면 위젯이 이 값을 보고 "대기 중" / "매치되었습니다!"를 띄운다
	void SetMatchState(EBCMatchState InMatchState);

	//이 플레이어가 지금 숫자를 입력할 차례인가? 남은 시간이 0이면(결과 공지 중 등) 누구의 차례도 아니다
	bool IsPlayerTurn(const ABCPlayerController* InPlayerController) const;

	//지금 차례 다음 사람부터 기회가 남은 플레이어를 찾아 턴을 주고 타이머를 시작함
	void StartNextTurn();

	//1초마다 타이머가 부름. 남은 시간을 1 줄이고 0이 되면 턴을 끝냄
	void UpdateTurnTimer();

	//턴 종료. 숫자를 입력해서 끝났든 시간이 다 돼서 끝났든 여기로 모인다
	void EndTurn();

	//승리/무승부 공지를 띄우고 턴을 멈춘 뒤, ResetGameDelay초 후에 ResetGame을 부름
	void EndGame(const FString& InResultString);

protected :
	//이번 게임의 정답. GameMode는 서버에만 있으니 이 값도 서버 메모리에만 있음 (클라이언트로 복제되지 않음)
	FString SecretNumberString;

	//UPROPERTY가 있어야 GC가 이 배열을 안다. 나간 플레이어의 PC가 지워지면 GC가 여기 포인터를 nullptr로 정리해 줘서 IsValid로 걸러낼 수 있음
	//없으면 지워진 메모리 주소가 그대로 남아(댕글링 포인터) IsValid를 불러도 크래시가 날 수 있음
	UPROPERTY()
	TArray<TObjectPtr<ABCPlayerController>> AllPlayerControllers;

	//지금 차례인 PC의 배열 인덱스. INDEX_NONE(-1)이면 아직 차례가 없음 → 다음 턴은 0번부터 확인
	int32 CurrentTurnIndex = INDEX_NONE;

	//활동 체크: 이번 턴에 숫자를 입력했는가. EndTurn에서 false면 시간 초과로 보고 기회를 1회 차감함
	bool bHasGuessedThisTurn = false;

	//1초마다 UpdateTurnTimer를 부르는 반복 타이머
	FTimerHandle TurnTimerHandle;

	//결과 공지 후 ResetGame을 한 번 부르는 타이머
	//"매치되었습니다!"를 보여 준 뒤 첫 판을 시작할 때도 같은 핸들로 ResetGame을 예약함 (둘 다 "ResetGame 예약"이라 하나로 충분)
	FTimerHandle ResetGameTimerHandle;

	//누가 접속하면 ConnectedNotificationTime초 뒤에 ClearConnectedNotification을 한 번 부르는 타이머
	//누가 또 들어오면 같은 핸들로 다시 걸어서 시간이 처음부터 다시 흐른다
	FTimerHandle ConnectedNotificationTimerHandle;

	//턴 하나의 제한 시간(초)
	//static constexpr: 객체마다 따로 갖는 값이 아니라 클래스 전체가 하나를 공유하는, 컴파일할 때 정해지는 상수
	//UPROPERTY가 아니라서 에디터/블루프린트에서는 바꿀 수 없고, 코드에서 숫자를 고쳐 다시 빌드해야 함
	static constexpr int32 TurnTimeLimit = 30;

	//결과 공지를 보여 준 뒤 새 게임을 시작하기까지 기다리는 시간(초)
	static constexpr float ResetGameDelay = 3.f;

	//새 판을 시작하는 데 필요한 최소 인원. 혼자 접속해 있을 때 턴이 돌아 기회만 깎이고 무승부가 반복되는 것을 막는다
	static constexpr int32 MinPlayerCountToStart = 2;

	//접속 공지를 보여 주는 시간(초)
	static constexpr float ConnectedNotificationTime = 3.f;

	//2명이 모였을 때 "매치되었습니다!"를 보여 주고 첫 판을 시작하기까지 기다리는 시간(초)
	static constexpr float MatchedNoticeTime = 2.f;
};
