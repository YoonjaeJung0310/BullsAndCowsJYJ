#include "Game/BCGameStateBase.h"

#include "Kismet/GameplayStatics.h" //UGameplayStatics::GetPlayerController
#include "Player/BCPlayerController.h"
#include "Net/UnrealNetwork.h" //DOREPLIFETIME 매크로

void ABCGameStateBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	//부모(AGameStateBase)가 등록하는 복제 변수들도 함께 등록되도록 Super를 먼저 부름
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(클래스, 변수): 이 변수를 조건 없이 복제 목록에 올린다. 서버에서 값이 바뀌면 클라이언트로 보내진다
	DOREPLIFETIME(ThisClass, CurrentTurnPlayerNameString);
	DOREPLIFETIME(ThisClass, RemainingTurnTime);
	DOREPLIFETIME(ThisClass, CurrentTurnRemainingGuessCount);
	DOREPLIFETIME(ThisClass, LastGuessNumberString);
	DOREPLIFETIME(ThisClass, LastGuessResultString);
	DOREPLIFETIME(ThisClass, MatchState);
}

//_Implementation: UFUNCTION(NetMulticast)를 달면 UHT가 MulticastRPCBroadcastLoginMessage() 본체(네트워크로 보내는 부분)를 자동으로 만듦
//우리는 "도착했을 때 실행할 내용"만 _Implementation에 씀
void ABCGameStateBase::MulticastRPCBroadcastLoginMessage_Implementation(const FString& InNameString)
{
	//HasAuthority(): 이 액터의 원본(권한)을 가진 쪽인가? GameState는 서버가 원본이라 서버에서는 true, 클라이언트에서는 false
	//→ == false 조건이라 아래 내용은 클라이언트에서만 실행됨
	if (HasAuthority() == false)
	{
		//GetPlayerController(월드, 0): 이 컴퓨터의 0번 로컬 플레이어의 PC. 클라이언트에는 자기 PC 하나만 있으니 "내 PC"가 나옴
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (IsValid(PC) == true)
		{
			ABCPlayerController* BCPC = Cast<ABCPlayerController>(PC);
			if (IsValid(BCPC) == true)
			{
				FString NotificationString = InNameString + TEXT(" has joined the game.");
				//RPC가 아니라 일반 함수 호출이다. 이미 클라이언트에서 실행 중이니 내 화면에 바로 찍는다
				BCPC->PrintChatMessageString(NotificationString);
			}
		}
	}
}
