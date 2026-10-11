//BullsAndCowsJYJ.h

#pragma once

#include "CoreMinimal.h"

//UCLASS가 아닌 일반 C++ 클래스. static 함수만 모아 둔 도우미(라이브러리)다
//static 함수라 객체를 만들 필요 없이 BullsAndCowsJYJFunctionLibrary::MyPrintString(...)처럼 클래스 이름으로 바로 부름
//함수 몸체를 클래스 안(헤더)에 바로 써서 따로 .cpp가 없음. 클래스 안에서 정의한 함수는 자동으로 inline이라
//여러 .cpp가 이 헤더를 include해도 "같은 함수가 여러 번 정의됐다"는 링크 에러가 나지 않는다
class BullsAndCowsJYJFunctionLibrary
{
public:
	//여기서 InWorldContextActor가 뭔 소리냐? 주소리? NO! 월드를 내가 특정할 수 있을 만한 월드 객체를 내가 얻어올 수 있을 만한 그런 엑터를 나한테 줘라
	//왜 엔진 PrintString 안 쓰고 새로 만드냐? 쓸데 없는 문자열을 기존 꺼가 생성해서

	//기본 인자: InTimeToDisplay를 안 넘기면 1초, InColor를 안 넘기면 하늘색(Cyan)
	static void MyPrintString(const AActor* InWorldContextActor, const FString& InString, float InTimeToDisplay = 1.f, FColor InColor = FColor::Cyan)
	{
		//GEngine: 엔진 전체를 가리키는 전역 포인터. 화면 디버그 메시지를 띄우는 함수가 여기에 있음
		if (IsValid(GEngine) == true && IsValid(InWorldContextActor) == true)
		{
			//GetNetMode(): 이 액터가 있는 월드의 네트워크 모드. NM_Standalone / NM_DedicatedServer / NM_ListenServer / NM_Client
			//클라이언트나 리슨 서버면 화면에 띄운다. 그 외(데디케이티드 서버, 스탠드얼론)는 아래 else로 가서 로그에만 남는다
			if (InWorldContextActor->GetNetMode() == NM_Client || InWorldContextActor->GetNetMode() == NM_ListenServer)
			{
				//첫 인자 Key: -1이면 매번 새 줄로 추가됨 (같은 양수 Key를 쓰면 그 줄을 덮어씀)
				GEngine->AddOnScreenDebugMessage(-1, InTimeToDisplay, InColor, InString);
			}
			else
			{
				//Log 단계: Output Log에 일반(흰색) 글로 남는다
				UE_LOG(LogTemp, Log, TEXT("%s"), *InString);
			}
		}
	}

};
