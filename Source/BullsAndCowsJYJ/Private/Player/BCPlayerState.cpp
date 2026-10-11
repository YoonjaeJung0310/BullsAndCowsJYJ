

#include "Player/BCPlayerState.h"

#include "Net/UnrealNetwork.h"

//멤버 이니셜라이저 목록(: 뒤): 생성자 본문 { }이 실행되기 전에 멤버 변수에 시작값을 넣음
//헤더에 선언한 순서(PlayerNameString → CurrentGuessCount → MaxGuessCount)대로 초기화됨
ABCPlayerState::ABCPlayerState()
	: PlayerNameString() //빈 문자열로 시작. 서버의 InitNewPlayer가 메뉴에서 받은 이름을 넣어 줌
	, CurrentGuessCount(0) //ResetGame도 0으로 되돌린다. 1에서 시작하면 2번만 쳐도 3/3이 돼서 무승부 판정이 일찍 난다
	, MaxGuessCount(3)
{
	//이 액터를 네트워크 복제 대상으로 켠다. 부모 APlayerState의 생성자도 이미 true로 켜 둔다
	bReplicates = true;
}

void ABCPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerNameString);
	DOREPLIFETIME(ThisClass, CurrentGuessCount);
	DOREPLIFETIME(ThisClass, MaxGuessCount); // Q.MaxGuessCount가 프로퍼티 레플게이션이 필요가 있을까?
	//있을 수도 있고 필요 없을 수도
	//특정 조건을 만족시켜서 늘어나거나 줄어들면 필요하지만 없으면 생성자에서 이미 3개를 했기 때문에
	//Bandwidth이라는 개념
}

FString ABCPlayerState::GetPlayerInfoString()
{
	//FString은 +로 이어 붙일 수 있음. FString::FromInt는 정수를 문자열로 바꿈 (1 → "1")
	//과제 명세의 [현재 시도 횟수 / 최대 시도 횟수] 예시에 맞춰 대괄호로 감싼다 → "Player1 [1/3]"
	FString PlayerInfoString = PlayerNameString + TEXT(" [") + FString::FromInt(CurrentGuessCount) + TEXT("/") + FString::FromInt(MaxGuessCount) + TEXT("]");
	return PlayerInfoString;
}
