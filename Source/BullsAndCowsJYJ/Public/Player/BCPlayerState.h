//BCPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BCPlayerState.generated.h"

//PlayerState: 플레이어 한 명의 정보(이름, 시도 횟수)를 담는 액터. 플레이어마다 하나씩 있음
//모든 클라이언트에 복제됨(엔진 기본값 bAlwaysRelevant = true) → 다른 사람의 정보도 볼 수 있음
//PlayerController는 주인 클라이언트에만 복제되는 것과 다르다
UCLASS()
class BULLSANDCOWSJYJ_API ABCPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	//생성자: 변수들의 시작값을 정함
	ABCPlayerState();

	//아래 세 변수를 복제 목록에 등록함
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//"Player1 [1/3]"처럼 이름 [쓴 기회/최대 기회] 형태의 표시용 문자열을 만듦
	FString GetPlayerInfoString();

	//이름 최대 글자 수. 메뉴(UBCMainMenu)의 입력 검사와 서버(GameMode)의 이름 정리가 같은 값을 쓰도록 여기 한 곳에 둔다
	static constexpr int32 MaxPlayerNameLength = 8;

public:
	//화면에 보일 이름. 메뉴에서 입력한 이름이 접속 주소(?BCName=)로 오고, 서버의 InitNewPlayer가 겹치지 않게 정리해 넣음
	UPROPERTY(Replicated)
	FString PlayerNameString;

	//지금까지 쓴 기회 수 (내 차례에 규칙에 맞는 숫자를 입력하거나, 시간 초과될 때마다 +1)
	UPROPERTY(Replicated)
	int32 CurrentGuessCount;

	//최대 기회 수 (생성자에서 3)
	UPROPERTY(Replicated)
	int32 MaxGuessCount;

};
