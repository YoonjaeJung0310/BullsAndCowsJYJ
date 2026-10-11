
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BCGameStateBase.generated.h"

//방(서버)이 지금 어느 단계인가. 대기 화면 위젯(UBCWaitingScreen)이 이 값을 보고 "대기 중" / "매치되었습니다!"를 띄운다
//UENUM(): 리플렉션에 등록해야 UPROPERTY(Replicated)로 복제할 수 있음
//uint8을 밑바탕 타입으로 써서 이 열거형 값은 1바이트로 저장됨
UENUM()
enum class EBCMatchState : uint8
{
	Waiting, //2명이 모이기를 기다리는 중
	Matched, //2명이 모여서 "매치되었습니다!"를 보여 주는 중. MatchedNoticeTime초 뒤 게임 시작
	Playing  //게임 진행 중 (승리·무승부 결과 공지 3초도 포함)
};

//GameState: 게임 전체가 공유하는 상태(지금 누구 차례인지, 남은 시간)를 담는 액터
//서버가 만들고 모든 클라이언트에 복제됨(엔진 기본값 bAlwaysRelevant = true). 서버에만 있는 GameMode와 달리 클라이언트도 읽을 수 있음
UCLASS()
class BULLSANDCOWSJYJ_API ABCGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

	public:
		//NetMulticast RPC: 서버에서 부르면 서버 자신과, 이 액터가 복제돼 있는 모든 클라이언트에서 _Implementation이 실행됨
		//Reliable: 연결이 유지되면 유실된 RPC를 재전송함. Unreliable은 재전송을 보장하지 않음
		//= FString(TEXT("XXXXXXX")): 기본 인자. 부를 때 이름을 안 넘기면 "XXXXXXX"가 들어감 (기본 인자는 선언(헤더)에만 씀)
		UFUNCTION(NetMulticast, Reliable)
		void MulticastRPCBroadcastLoginMessage(const FString& InNameString = FString(TEXT("XXXXXXX")));

		//어떤 변수를 복제할지 엔진에 등록하는 함수. UPROPERTY(Replicated)를 단 변수는 .cpp의 이 함수에서 DOREPLIFETIME으로 등록함
		virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	public:
		//지금 차례인 플레이어 이름. 모두가 같은 값을 봐야 하는 게임 전체 정보라 PlayerState가 아니라 GameState에 둔다
		UPROPERTY(Replicated)
		FString CurrentTurnPlayerNameString;

		//이번 턴 남은 시간(초). 서버(GameMode)만 1초마다 줄이고, 클라는 복제받은 값을 타이머 위젯에 보여 주기만 함
		//모두 서버의 시간을 복제받음. 네트워크 지연 때문에 화면 반영은 조금 차이 날 수 있음. 0이면 턴이 없는 상태
		UPROPERTY(Replicated)
		int32 RemainingTurnTime = 0;

		//지금 차례인 사람의 남은 기회(최대 - 쓴 횟수). 왼쪽 위 전광판의 "기회 N번 남음"에 씀
		//턴이 시작될 때(StartNextTurn) 서버가 넣음. 한 턴 안에서는 바뀌지 않는다 (숫자를 입력하면 바로 다음 턴으로 넘어가니까)
		UPROPERTY(Replicated)
		int32 CurrentTurnRemainingGuessCount = 0;

		//마지막으로 판정한 숫자와 결과. 가운데 전광판에 크게 "321", 작게 "1S2B"로 띄운다
		//판정할 때 서버가 넣고, 새 판(ResetGame)에서 비움. 모두가 같은 값을 봐야 해서 GameState에 둔다
		UPROPERTY(Replicated)
		FString LastGuessNumberString;

		UPROPERTY(Replicated)
		FString LastGuessResultString;

		//방의 단계. 서버(GameMode)만 바꾸고, 클라이언트의 대기 화면·턴 위젯은 읽기만 함
		UPROPERTY(Replicated)
		EBCMatchState MatchState = EBCMatchState::Waiting;
};
