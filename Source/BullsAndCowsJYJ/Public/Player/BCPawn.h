
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BCPawn.generated.h"

//Pawn: 컨트롤러가 빙의(Possess)해서 조종하는 "몸" 액터. 컨트롤러(PC)는 "머리", 폰은 "몸"
UCLASS()
class BULLSANDCOWSJYJ_API ABCPawn : public APawn
{
	GENERATED_BODY()
	//덮어쓴(override) 함수가 없으니 BeginPlay, PossessedBy 같은 함수는 부모 APawn의 것이 그대로 불림
};
