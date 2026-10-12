//BCGameModeBase.cpp

#include "Game/BCGameModeBase.h"

#include "Game/BCGameStateBase.h"
#include "Player/BCPlayerController.h"
#include "EngineUtils.h" //TActorIterator가 들어 있는 헤더 (BroadcastChatMessageString에서 사용)
#include "Player/BCPlayerState.h"
#include "Kismet/GameplayStatics.h" //UGameplayStatics::ParseOption (InitNewPlayer에서 접속 옵션 읽기)


//로그인 순서: (엔진) Login → InitNewPlayer(여기서 이름) → PostLogin → OnPostLogin
FString ABCGameModeBase::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	//부모(AGameModeBase::InitNewPlayer)가 시작 위치 찾기, 엔진 쪽 이름(?Name=) 넣기 등을 먼저 함. 돌려주는 글이 비어 있지 않으면 접속 실패 사유
	FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	//엔진의 ?Name= 이 아니라 우리 키 ?BCName= 을 쓰는 이유:
	//클라이언트 엔진이 접속 과정(PendingNetGame)에서 ?Name= 값을 온라인 서브시스템 닉네임("컴퓨터이름-GUID")으로 덮어씀
	//→ ?Name= 으로 보내면 메뉴에서 입력한 이름이 서버까지 오지 않는다 (엔진 소스 PendingNetGame.cpp, URL.cpp의 AddOption)
	//ParseOption(옵션 문자열, 키): "?BCName=윤재?..."에서 키에 해당하는 값("윤재")을 꺼낸다. 없으면 빈 문자열 → MakeUniquePlayerName이 "Player"로 채운다
	ABCPlayerState* BCPS = NewPlayerController->GetPlayerState<ABCPlayerState>();
	if (IsValid(BCPS) == true)
	{
		//이 시점엔 새 PC가 아직 AllPlayerControllers에 없어서(OnPostLogin에서 넣음) 이름 중복 검사에 자기 자신이 섞이지 않는다
		//PlayerNameString은 Replicated라 서버에서 정하면 모든 클라이언트가 같은 이름을 본다
		BCPS->PlayerNameString = MakeUniquePlayerName(UGameplayStatics::ParseOption(Options, TEXT("BCName")));
	}

	return ErrorMessage;
}

//새 플레이어가 접속하면: 접속 공지 → 배열에 등록 → 모두에게 입장 알림 → (2명이 모였으면) "매치되었습니다!" 후 첫 판 예약
//이름은 그 전에 InitNewPlayer에서 정해 둔다
void ABCGameModeBase::OnPostLogin(AController* NewPlayer)
{
	//부모 버전을 먼저 부름. AGameModeBase::OnPostLogin이 블루프린트의 Event OnPostLogin(C++ 이름 K2_PostLogin)을 불러 준다
	Super::OnPostLogin(NewPlayer);

	//NewPlayer는 부모 타입(AController*)으로 들어옴. 우리가 만든 변수·함수를 쓰려면 ABCPlayerController로 Cast함
	//Cast는 타입이 안 맞으면 nullptr을 돌려주므로 바로 IsValid로 확인함
	ABCPlayerController* BCPlayerController = Cast<ABCPlayerController>(NewPlayer);
	if (IsValid(BCPlayerController) == true)
	{
		//NotificationText는 Replicated 변수라 서버에서 값을 넣으면 이 플레이어의 클라이언트로 복제되어 공지 위젯에 뜬다
		BCPlayerController->SetNotificationText(FText::FromString(TEXT("Connected to the game server.")));
		//접속 공지는 ConnectedNotificationTime초만 보여 주고 지움 (그냥 두면 첫 판이 끝날 때까지 공지 위젯에 남아 있었다)
		//마지막 인자 false = 한 번만 실행. 같은 핸들이라 누가 또 들어오면 이전 예약은 지워지고 다시 3초를 센다
		GetWorldTimerManager().SetTimer(ConnectedNotificationTimerHandle, this, &ThisClass::ClearConnectedNotification, ConnectedNotificationTime, false);

		//접속한 순서대로 배열 끝에 붙임. 이 배열의 순서가 그대로 턴 순서가 됨 (StartNextTurn)
		AllPlayerControllers.Add(BCPlayerController);

		//GetPlayerState<T>(): PC에 붙어 있는 PlayerState를 T 타입으로 Cast해서 돌려줌 (이름은 InitNewPlayer에서 이미 들어 있음)
		ABCPlayerState* BCPS = BCPlayerController->GetPlayerState<ABCPlayerState>();

		//GetGameState<T>(): GameMode가 만들어 둔 GameState를 T 타입으로 Cast해서 돌려줌
		ABCGameStateBase* BCGameStateBase = GetGameState<ABCGameStateBase>();

		//BCPS가 없으면 BCPS->PlayerNameString에서 크래시가 나니 둘 다 확인함
		if (IsValid(BCGameStateBase) == true && IsValid(BCPS) == true)
		{
			BCGameStateBase->MulticastRPCBroadcastLoginMessage(BCPS->PlayerNameString);
		}

		//턴이 돌고 있지도 않고 결과 공지 후 리셋을 기다리는 중도 아니면(= 첫 플레이어가 들어왔거나, 다 나가서 멈춰 있던 방) 바로 턴을 시작함
		//그 외에는 새로 들어온 사람이 AllPlayerControllers 끝에 붙어 있다가 차례가 오면 턴을 받음
		//+ 접속자가 MinPlayerCountToStart(2)명 이상일 때만 시작함(CanStartGame). 혼자면 대기 화면(Waiting)에서 다음 사람을 기다린다
		//+ 바로 턴을 주지 않고 "매치되었습니다!"(Matched)를 MatchedNoticeTime초 보여 준 뒤 ResetGame으로 첫 판을 연다 (새 정답·횟수 초기화·턴 시작을 ResetGame이 다 함)
		//ResetGameTimerHandle로 예약하니, 그동안 또 누가 들어와도 아래 IsTimerActive 검사에 걸려 두 번 예약되지 않는다
		//+ 예외: 이번 접속으로 인원이 막 MinPlayerCountToStart(2)명이 됐다면, 결과 공지 중(ResetGameTimerHandle이 도는 중)이어도 매치를 건다
		//상대가 나가 혼자 이긴 뒤 3초 안에 새 사람이 들어온 경우다. 이걸 안 하면 "매치되었습니다!" 없이 바로 새 판이 시작됨
		//같은 핸들에 SetTimer를 다시 걸면 EndGame이 걸어 둔 예약은 지워지고 MatchedNoticeTime초 뒤 ResetGame으로 바뀐다
		bool bJustReachedMinPlayerCount = GetValidPlayerControllers().Num() == MinPlayerCountToStart;
		if (GetWorldTimerManager().IsTimerActive(TurnTimerHandle) == false && (GetWorldTimerManager().IsTimerActive(ResetGameTimerHandle) == false || bJustReachedMinPlayerCount == true) && CanStartGame() == true)
		{
			SetMatchState(EBCMatchState::Matched);
			GetWorldTimerManager().SetTimer(ResetGameTimerHandle, this, &ThisClass::ResetGame, MatchedNoticeTime, false);
		}
	}
}

//나간 사람 처리: 배열 자리 비우기 → (게임 중일 때만) 한 명 남았으면 그 사람 승리 / 여럿 남았고 나간 사람 차례였으면 바로 다음 턴
void ABCGameModeBase::Logout(AController* Exiting)
{
	//부모 처리(블루프린트의 Event OnLogout(C++ 이름 K2_OnLogout) 등)를 먼저 실행함
	Super::Logout(Exiting);

	//Logout은 PC의 Destroyed() 안에서, "삭제 예정" 표시가 붙기 전에 불림 → 여기서는 아직 IsValid가 true다
	//Cast가 실패하면(nullptr) 아래 Find가 앞서 비워 둔 nullptr 자리를 찾아 버리므로 먼저 걸러 낸다
	ABCPlayerController* ExitingPlayerController = Cast<ABCPlayerController>(Exiting);
	if (IsValid(ExitingPlayerController) == false)
	{
		return;
	}

	//우리 배열에 있는 PC인지 찾는다. Find는 같은 값이 있으면 그 인덱스, 없으면 INDEX_NONE(-1)
	int32 ExitingIndex = AllPlayerControllers.Find(ExitingPlayerController);
	if (ExitingIndex == INDEX_NONE)
	{
		return;
	}

	//자리를 비우기 "전에" 그 사람 차례였는지 확인해 둔다. 비운 뒤에는 IsPlayerTurn이 항상 false가 됨
	bool bWasPlayerTurn = IsPlayerTurn(ExitingPlayerController);

	//배열에서 빼지(RemoveAt) 않고 자리만 nullptr로 비움
	//빼면 뒤 사람들의 인덱스가 하나씩 당겨져 CurrentTurnIndex가 엉뚱한 사람을 가리키게 됨
	//(PC는 이 함수가 끝난 뒤에 지워지므로, 여기서 비워야 아래 판정들이 나간 사람을 빼고 센다)
	AllPlayerControllers[ExitingIndex] = nullptr;

	//턴이 돌고 있지 않을 때(대기 중·"매치되었습니다!" 표시 중·결과 공지 중)
	if (GetWorldTimerManager().IsTimerActive(TurnTimerHandle) == false)
	{
		//"매치되었습니다!"를 보여 주는 중에 상대가 나가 인원이 모자라면, 남은 시간(최대 MatchedNoticeTime초)을 기다리지 않고 바로 대기 화면으로 돌린다
		//결과 공지 중이면 그대로 둔다 → 예약된 ResetGame이 남은 인원을 보고 새 판을 시작하거나 대기 화면으로 돌린다
		ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
		if (IsValid(BCGS) == true && BCGS->MatchState == EBCMatchState::Matched && CanStartGame() == false)
		{
			GetWorldTimerManager().ClearTimer(ResetGameTimerHandle);
			SetMatchState(EBCMatchState::Waiting);
		}
		return;
	}

	//게임 중에 한 명만 남았으면 그 사람이 승리. EndGame → ResetGameDelay초 뒤 ResetGame → 인원이 모자라니 대기 화면으로 감
	TArray<ABCPlayerController*> RemainingPlayerControllers = GetValidPlayerControllers();
	if (RemainingPlayerControllers.Num() == 1)
	{
		ABCPlayerState* WinnerBCPS = RemainingPlayerControllers[0]->GetPlayerState<ABCPlayerState>();
		if (IsValid(WinnerBCPS) == true)
		{
			EndGame(WinnerBCPS->PlayerNameString + TEXT(" 승리!"));
		}
		return;
	}

	//여럿 남았고 나간 사람이 지금 차례였으면 남은 시간을 기다리지 않고 바로 턴을 넘김
	//나간 사람은 숫자를 칠 수 없으니 시간 초과 차감·안내(EndTurn)는 하지 않는다
	if (bWasPlayerTurn == true)
	{
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);

		//남은 사람이 모두 기회를 다 썼다면 무승부. 승리는 나올 수 없으니 스트라이크 0, 승자 PC는 쓰지 않으니 nullptr로 판정함
		if (JudgeGame(nullptr, 0) == false)
		{
			StartNextTurn();
		}
	}
}

FString ABCGameModeBase::MakeUniquePlayerName(const FString& InNameString) const
{
	//TrimStartAndEnd: 앞뒤 공백을 뺀 새 문자열 / Left(N): 앞에서 N글자만 남긴 새 문자열
	//메뉴에서도 검사하지만, 서버는 클라이언트가 보낸 값을 그대로 믿지 않고 한 번 더 정리함
	FString BaseNameString = InNameString.TrimStartAndEnd().Left(ABCPlayerState::MaxPlayerNameLength);
	if (BaseNameString.IsEmpty() == true)
	{
		BaseNameString = TEXT("Player");
	}

	//이미 접속한 사람 중에 같은 이름이 있는가? 이 함수 안에서만 쓰니 람다로 만듦
	//[this]: 람다 안에서 멤버 변수(AllPlayerControllers)를 쓰려고 this를 캡처함
	auto IsPlayerNameTaken = [this](const FString& InCheckNameString)
	{
		for (const auto& BCPlayerController : AllPlayerControllers)
		{
			if (IsValid(BCPlayerController) == false)
			{
				continue;
			}

			ABCPlayerState* BCPS = BCPlayerController->GetPlayerState<ABCPlayerState>();
			//FString의 ==는 대소문자를 구분하지 않는다 → "Mir"와 "mir"도 같은 이름으로 본다
			if (IsValid(BCPS) == true && BCPS->PlayerNameString == InCheckNameString)
			{
				return true;
			}
		}
		return false;
	};

	//겹치면 뒤에 2, 3, 4…를 붙여 가며 비어 있는 이름을 찾는다 (윤재가 있으면 윤재2)
	FString NameString = BaseNameString;
	int32 NameSuffix = 2;
	while (IsPlayerNameTaken(NameString) == true)
	{
		NameString = BaseNameString + FString::FromInt(NameSuffix);
		NameSuffix++;
	}
	return NameString;
}

void ABCGameModeBase::SetMatchState(EBCMatchState InMatchState)
{
	//MatchState는 GameState의 Replicated 변수라 서버에서 바꾸면 모든 클라이언트의 대기 화면 위젯이 같은 단계를 본다
	ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == true)
	{
		BCGS->MatchState = InMatchState;
	}
}

//후보 1~9를 담아 두고, 무작위로 하나 뽑아 붙이고 후보에서 빼는 것을 3번 반복함
FString ABCGameModeBase::GenerateSecretNumber()
{
	//후보 숫자 1~9를 배열에 담음 (0부터가 아니라 1부터 시작하므로 0은 후보에 없음)
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	//난수 씨앗(seed) 설정. 씨앗이 같으면 매번 똑같은 순서의 난수가 나옴
	//현재 시각의 Tick 값을 씨앗으로 넣어서 실행할 때마다 씨앗이 달라진다 (RandInit은 int32를 받으니 int64인 Tick 값이 잘려서 들어감)
	FMath::RandInit(FDateTime::Now().GetTicks());

	FString Result;
	for (int32 i = 0; i < 3; i++)
	{
		//RandRange(최소, 최대)는 양 끝을 모두 포함함 → 0 ~ Num()-1 = 배열에서 쓸 수 있는 인덱스 범위 전체
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		//뽑힌 숫자(int32)를 문자열로 바꿔 Result 뒤에 이어 붙임
		Result.Append(FString::FromInt(Numbers[Index]));
		//뽑은 숫자는 후보에서 뺀다. 안 빼면 같은 숫자가 또 뽑혀 337 같은 중복 정답이 나옴
		Numbers.RemoveAt(Index);
	}

	return Result;
}

//검사 순서: 길이가 3인가? → 글자마다 (1~9 숫자인가? → 앞에서 나온 숫자와 겹치지 않는가?)
//OutInvalidReasonString은 FString&(const 없는 참조)라서 함수 안에서 넣은 값이 호출한 쪽 변수에 그대로 남는다 → 반환값 말고도 결과를 하나 더 돌려주는 "출력 매개변수"
bool ABCGameModeBase::IsGuessNumberString(const FString& InNumberString, FString& OutInvalidReasonString)
{
	//기본값은 false. 모든 검사를 통과했을 때만 아래에서 true로 바뀐다
	bool bCanPlay = false;
	//do { ... } while(false): 본문을 딱 한 번만 실행하는 반복문
	//반복문이라서 중간에 break를 쓸 수 있음 → 길이 검사에 걸리면 남은 검사를 건너뛰고 while(false) 아래로 바로 빠져나감
	//(단, 아래 for 안에 있는 break는 for만 빠져나감. 그 break 위의 주석 참고)
	do
	{
		//Len(): 문자열의 글자 수
		if (InNumberString.Len() != 3)
		{
			//"12", "1234", "abcd"처럼 길이부터 틀린 입력
			OutInvalidReasonString = TEXT("3자리 숫자를 입력해주세요.");
			break;
		}
		//아래 반복문에서 하나라도 걸리면 false로 바뀐다
		bool bIsUnique = true;
		//TSet: 같은 값을 한 번만 담는 집합. Contains로 "이미 나온 글자인가?"를 확인함
		//(해시로 찾기 때문에 원소가 많을수록 배열을 처음부터 훑는 것보다 빠르다. 여기처럼 3글자뿐이면 속도 차이는 거의 없음)
		TSet<TCHAR> UniqueDigits;
		//범위 기반 for문: 문자열의 글자(TCHAR)를 앞에서부터 하나씩 C에 꺼내 옴
		for (TCHAR C : InNumberString)
		{
			//글자 코드가 '1'~'9' 범위 밖이면 걸러 낸다. 문자, '0', 공백이 모두 여기서 걸린다
			//FChar::IsDigit을 쓰지 않는 이유: 안에서 iswdigit을 불러 전각 숫자(３)·위 첨자(²) 같은 유니코드 숫자도 true가 됨
			//→ 그런 입력이 검사를 통과해 항상 OUT으로 판정되고 기회가 깎였다
			//TEXT('1'): TCHAR(와이드 문자) 리터럴. C가 TCHAR라서 같은 타입끼리 비교함
			if (C < TEXT('1') || C > TEXT('9'))
			{
				//"1a2"처럼 문자가 섞였거나 "102"처럼 0이 들어간 입력 (정답은 1~9로만 만듦)
				OutInvalidReasonString = TEXT("1~9 사이의 숫자만 입력해주세요.");
				bIsUnique = false;
				//이 break는 가장 가까운 반복문인 for만 빠져나감. do-while은 그대로 이어져서 아래 bCanPlay = bIsUnique 줄이 실행됨
				break;
			}
			//이미 나온 숫자면 중복이므로 숫자 야구 입력으로 인정하지 않는다
			if (UniqueDigits.Contains(C))
			{
				OutInvalidReasonString = TEXT("중복되지 않은 숫자를 입력해주세요.");
				bIsUnique = false;
				break;
			}
			//검사를 통과한 글자는 집합에 넣어 둔다 → 다음 글자부터는 이 글자와 겹치는지 확인할 수 있음
			UniqueDigits.Add(C);
		}
		//반복문을 다 돌고도 걸리지 않았을 때(모두 1~9, 중복 없음)에만 true. 반복문 안에 두면 조건이 한 번도 만족되지 않는다
		bCanPlay = bIsUnique;
	}
	while (false);

	return bCanPlay;
}

//스트라이크(S): 같은 자리에 같은 숫자 / 볼(B): 자리는 다르지만 정답 어딘가에 있는 숫자
//예) 정답 123, 입력 132 → 1은 S, 3과 2는 자리만 달라서 B → "1S2B"
//정답은 매개변수로 받지 않고 멤버 SecretNumberString을 바로 읽음
FString ABCGameModeBase::CheckAnswer(const FString& InGuessNumberString)
{
	//쉼표로 같은 타입 변수 두 개를 한 줄에 선언하고 둘 다 0으로 시작함
	int32 StrikeCount = 0, BallCount = 0;

	//0번 ~ 2번 자리를 같은 위치끼리 비교함. FString[i]는 i번째 글자(TCHAR)를 돌려줌
	for (int32 i = 0; i < 3; ++i)
	{
		if (SecretNumberString[i] == InGuessNumberString[i])
		{

			StrikeCount++;
		}
		else
		{
			//Contains에 넘기기 위해 TCHAR 한 글자를 Printf의 %c로 한 글자짜리 FString으로 만듦
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			//같은 자리는 위에서 이미 다르다고 확인했으니, 정답 안에 있기만 하면 다른 자리에 있다는 뜻 → 볼
			if (SecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;
			}
		}
	}

	//S도 B도 없으면 숫자를 붙이지 않고 "OUT"
	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	//Printf: %d 자리에 뒤의 정수가 순서대로 들어감 → 예: "1S2B"
	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ABCGameModeBase::BeginPlay()
{
	//부모(AActor 계열)의 BeginPlay를 먼저 실행함. AActor::BeginPlay가 컴포넌트들의 BeginPlay와 블루프린트의 Event BeginPlay를 불러 준다
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
	//UE_LOG(카테고리, 단계, 형식, 값): Output Log에 글을 남긴다. Error 단계는 빨간색으로 보임
	//LogTemp는 엔진이 기본으로 주는 임시 카테고리. %s에 FString을 넣을 때는 앞에 *를 붙여 TCHAR* 문자열로 바꿔 준다
	UE_LOG(LogTemp, Error, TEXT("%s"), *SecretNumberString);
}

//흐름: PlayerState 얻기 → 내 차례인가? → 규칙에 맞는 입력인가? → 판정 → 횟수 +1 → 결과 방송 → 게임 끝? → 턴 넘기기
void ABCGameModeBase::PrintChatMessageString(ABCPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	//이름과 [쓴 기회/최대 기회]는 서버에서 붙여 줌
	ABCPlayerState* BCPS = InChattingPlayerController->GetPlayerState<ABCPlayerState>();
	if (IsValid(BCPS) == false)
	{
		return;
	}

	//내 차례가 아니면(남의 턴, 기회를 다 쓴 사람, 결과 공지 중) 판정하지 않고 일반 채팅으로만 보여 줌
	//→ 시간이 0이 됐거나 기회가 0회인 사람이 숫자를 쳐도 서버가 판정을 받아 주지 않는다
	if (IsPlayerTurn(InChattingPlayerController) == false)
	{
		//일반 채팅은 시도 횟수 없이 이름만 붙임 → "윤재: 하이" (숫자 야구 입력일 때만 "윤재 [1/3]: 123 -> 0S1B"처럼 횟수를 붙임)
		BroadcastChatMessageString(BCPS->PlayerNameString + TEXT(": ") + InChatMessageString);
		return;
	}

	//내 차례에 친 글은 전부 숫자 야구 입력으로 검사함
	//규칙에 안 맞으면 본인에게만 이유를 알려 주고 끝. 기회도 안 깎고 턴도 안 넘기니 남은 시간 안에 다시 입력하면 됨
	FString InvalidReasonString;
	if (IsGuessNumberString(InChatMessageString, InvalidReasonString) == false)
	{
		FString CombinedMessageString = BCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString + TEXT(" -> 다시 입력하세요! ") + InvalidReasonString;
		//Client RPC는 이 PC의 주인 클라이언트 한 명에게만 감 (모두에게 보내는 BroadcastChatMessageString과 다름)
		InChattingPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
		return;
	}

	//규칙 검사를 통과한 입력만 여기까지 옴. 서버에만 있는 정답(SecretNumberString)과 비교함
	FString JudgeResultString = CheckAnswer(InChatMessageString);

	//가운데 전광판에 띄울 숫자와 결과를 GameState에 넣음 → 복제돼서 모든 클라이언트의 전광판(UBCScoreBoard)이 같은 값을 보여 줌
	ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == true)
	{
		BCGS->LastGuessNumberString = InChatMessageString;
		BCGS->LastGuessResultString = JudgeResultString;
	}

	IncreaseGuessCount(InChattingPlayerController);
	//활동 체크: 이번 턴에 숫자를 입력했다 → EndTurn에서 시간 초과 차감을 하지 않는다
	bHasGuessedThisTurn = true;

	//횟수를 올린 "뒤에" 이름을 붙여야 이번 입력까지 센 횟수가 보임 (예: Player1(1/3): 123 -> 0S1B)
	BroadcastChatMessageString(BCPS->GetPlayerInfoString() + TEXT(": ") + InChatMessageString + TEXT(" -> ") + JudgeResultString, InChatMessageString, JudgeResultString);

	//"OUT"은 첫 글자가 숫자가 아니라 Atoi가 0을 돌려줌 → 스트라이크 0개로 처리됨
	//Left(1): 앞에서 1글자만 잘라낸 FString ("2S1B" → "2"). Atoi: 숫자 문자열을 정수로 바꿈 ("2" → 2)
	int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));

	//숫자 한 번 입력했으니 승리·무승부 판정도 한 번만 함
	if (JudgeGame(InChattingPlayerController, StrikeCount) == true)
	{
		return;
	}

	//게임이 안 끝났으면 입력했으니 턴을 넘김
	EndTurn();
}

//채팅을 보낸 플레이어의 PlayerState에서 CurrentGuessCount를 1 올린다 (PlayerState는 플레이어마다 하나씩이라 횟수도 플레이어별로 따로 쌓인다)
void ABCGameModeBase::IncreaseGuessCount(ABCPlayerController* InChattingPlayerController)
{
	ABCPlayerState* BCPS = InChattingPlayerController->GetPlayerState<ABCPlayerState>();
	if (IsValid(BCPS) == true)
	{
		//CurrentGuessCount는 Replicated라 서버에서 올리면 클라이언트들의 PlayerState에도 복제됨
		BCPS->CurrentGuessCount++;
	}
}

//ResetGameTimerHandle에 예약돼서 불린다: EndGame(결과 공지 ResetGameDelay초 뒤), OnPostLogin(매치 공지 MatchedNoticeTime초 뒤)
void ABCGameModeBase::ResetGame()
{
	SecretNumberString = GenerateSecretNumber();
	//새 정답도 Output Log에서 확인할 수 있게 BeginPlay와 똑같이 찍는다
	UE_LOG(LogTemp, Error, TEXT("%s"), *SecretNumberString);

	//범위 기반 for + const auto&: auto는 원소 타입(TObjectPtr<ABCPlayerController>)을 컴파일러가 알아서 정함
	//&(참조)라서 원소를 복사하지 않고, const라서 원소(포인터 자체)를 바꾸지 않는다. 포인터가 가리키는 PC의 값은 바꿀 수 있음
	for (const auto& BCPlayerController : AllPlayerControllers)
	{
		//나간 플레이어의 PC는 GC가 nullptr로 정리해 두므로 건너뛴다
		if (IsValid(BCPlayerController) == false)
		{
			continue;
		}

		ABCPlayerState* BCPS = BCPlayerController->GetPlayerState<ABCPlayerState>();
		if (IsValid(BCPS) == true)
		{
			//시도 횟수를 0으로 되돌린다 → 모두 다시 기회 3번
			BCPS->CurrentGuessCount = 0;
		}

		//결과 공지는 ResetGameDelay초 동안 보여 줬으니 지움
		BCPlayerController->SetNotificationText(FText::GetEmpty());
	}

	//가운데 전광판도 새 판이니 비움
	ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == true)
	{
		BCGS->LastGuessNumberString.Empty();
		BCGS->LastGuessResultString.Empty();
		//다음 판을 기다릴 때도 지난 턴 이름·시간·기회가 남아 있지 않도록 같이 비워 줌
		BCGS->CurrentTurnPlayerNameString.Empty();
		BCGS->RemainingTurnTime = 0;
		BCGS->CurrentTurnRemainingGuessCount = 0;
	}

	//턴 순서도 처음으로 되돌린다. INDEX_NONE에서 다음 사람을 찾으니 0번(Player1)부터 다시 시작함
	CurrentTurnIndex = INDEX_NONE;
	//새 판은 아직 아무도 입력하지 않은 상태임. 인원이 모자라 대기할 때도 같은 시작값으로 둠
	bHasGuessedThisTurn = false;
	//2명 이상 남아 있으면 게임 진행(Playing)으로 바꾸고 바로 시작함
	if (CanStartGame() == true)
	{
		SetMatchState(EBCMatchState::Playing);
		BroadcastChatMessageString(TEXT("새 게임을 시작합니다!"));
		StartNextTurn();
	}
	//한 명만 남았으면(상대가 나감) 대기 화면(Waiting)으로 돌아가 다음 사람을 기다린다. 누가 들어오면 OnPostLogin이 다시 매치를 잡는다
	//인원이 모자라면 새 게임 안내를 보내지 않고 대기 화면으로 감
	else
	{
		SetMatchState(EBCMatchState::Waiting);
	}
}

//판정 순서: 3S면 승리 → 아니면 모든 플레이어가 기회를 다 썼는지 보고 무승부 → 둘 다 아니면 false(게임 계속)
bool ABCGameModeBase::JudgeGame(ABCPlayerController* InChattingPlayerController, int InStrikeCount)
{
	//상수를 왼쪽에 둔 비교. =를 하나 빠뜨려 3 = InStrikeCount가 되면 상수에 대입할 수 없어 컴파일 에러가 난다
	if (3 == InStrikeCount)
	{
		//승자 이름을 얻으려고 이 분기에서만 InChattingPlayerController를 사용함
		ABCPlayerState* BCPS = InChattingPlayerController->GetPlayerState<ABCPlayerState>();
		if (IsValid(BCPS) == true)
		{
			FString CombinedMessageString = BCPS->PlayerNameString + TEXT(" 승리!");
			//EndGame이 결과 공지와 리셋 타이머를 같이 처리해 줌
			EndGame(CombinedMessageString);
		}
		return true;
	}

	//3S가 아니면 무승부인지 본다: 기회가 남은 사람이 한 명이라도 있으면 게임은 계속됨
	bool bIsDraw = true;
	for (const auto& BCPlayerController : AllPlayerControllers)
	{
		if (IsValid(BCPlayerController) == false)
		{
			continue;
		}

		ABCPlayerState* BCPS = BCPlayerController->GetPlayerState<ABCPlayerState>();
		if (IsValid(BCPS) == true)
		{
			if (BCPS->CurrentGuessCount < BCPS->MaxGuessCount)
			{
				//기회가 남은 사람을 한 명 찾았으면 무승부가 아니다. 더 볼 필요가 없으니 반복문을 끝냄
				bIsDraw = false;
				break;
			}
		}
	}

	//반복문을 끝까지 돌았는데도 bIsDraw가 true = 모든 플레이어가 기회를 다 썼다
	if (true == bIsDraw)
	{
		EndGame(TEXT("무승부!"));
		return true;
	}

	return false;
}

//서버에는 접속한 모든 플레이어의 PC가 있음 → PC마다 Client RPC를 보내면 각자의 화면에 한 줄씩 뜬다
void ABCGameModeBase::BroadcastChatMessageString(const FString& InChatMessageString, const FString& InGuessNumberString, const FString& InGuessResultString)
{
	//TActorIterator<T>: 월드에 있는 T 타입 액터를 하나씩 돎. It이 유효한 동안 반복하고, ++It으로 다음 액터로 넘어감
	//AllPlayerControllers 배열이 아니라 지금 월드에 실제로 있는 PC를 찾아서 돎
	for (TActorIterator<ABCPlayerController> It(GetWorld()); It; ++It)
	{
		//*It: 반복자가 지금 가리키는 액터 포인터를 꺼낸다
		ABCPlayerController* BCPlayerController = *It;
		if (IsValid(BCPlayerController) == true)
		{
			BCPlayerController->ClientRPCPrintChatMessageString(InChatMessageString, InGuessNumberString, InGuessResultString);
		}
	}
}

//NotificationText는 PC에 있고 PC는 주인 클라이언트에게만 복제되므로, 모두에게 보이려면 PC마다 하나씩 넣어야 함
void ABCGameModeBase::BroadcastNotificationText(const FText& InNotificationText)
{
	for (const auto& BCPlayerController : AllPlayerControllers)
	{
		if (IsValid(BCPlayerController) == true)
		{
			BCPlayerController->SetNotificationText(InNotificationText);
		}
	}
}

void ABCGameModeBase::ClearConnectedNotification()
{
	//결과 공지(승리/무승부)를 보여 주는 중이면 건드리지 않는다. 그 공지는 3초 뒤 ResetGame이 지움
	if (GetWorldTimerManager().IsTimerActive(ResetGameTimerHandle) == true)
	{
		return;
	}

	//결과 공지 중이 아니면 공지 위젯에 떠 있을 수 있는 건 접속 공지뿐이라 전부 지워도 됨
	//(공지 문구를 바꾸는 곳: OnPostLogin=접속 공지, EndGame=결과 공지, ResetGame·여기=지우기. 다른 공지를 새로 만들면 이 가정을 다시 봐야 함)
	BroadcastNotificationText(FText::GetEmpty());
}

bool ABCGameModeBase::CanStartGame() const
{
	//배열에는 나간 사람 자리(nullptr)도 남아 있으니 Num()이 아니라 아직 접속해 있는 PC만 센다
	return GetValidPlayerControllers().Num() >= MinPlayerCountToStart;
}

TArray<ABCPlayerController*> ABCGameModeBase::GetValidPlayerControllers() const
{
	TArray<ABCPlayerController*> ValidPlayerControllers;
	for (const auto& BCPlayerController : AllPlayerControllers)
	{
		if (IsValid(BCPlayerController) == true)
		{
			//.Get(): TObjectPtr에서 일반 포인터를 꺼내 담음 (돌려주는 배열이 ABCPlayerController* 배열이라서)
			ValidPlayerControllers.Add(BCPlayerController.Get());
		}
	}
	return ValidPlayerControllers;
}

//함수 뒤의 const: 이 함수 안에서는 멤버 변수를 바꾸지 않는다는 약속. 어기면 컴파일 에러가 난다
bool ABCGameModeBase::IsPlayerTurn(const ABCPlayerController* InPlayerController) const
{
	//남은 시간은 GameState에 있음. 0이면 턴이 없는 상태(결과 공지 중, 리셋 대기)라 아무도 입력할 수 없음
	const ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == false || BCGS->RemainingTurnTime <= 0)
	{
		return false;
	}

	//IsValidIndex로 인덱스가 배열 범위 안인지 먼저 확인함. -1(INDEX_NONE)처럼 범위 밖 인덱스로 [ ]에 접근하면 크래시가 난다
	//&&(그리고)는 앞이 false면 뒤를 계산하지 않는다(단락 평가) → 범위 밖이면 [ ] 접근까지 가지 않는다
	return AllPlayerControllers.IsValidIndex(CurrentTurnIndex) == true && AllPlayerControllers[CurrentTurnIndex] == InPlayerController;
}

//EndTurn(턴 종료), Logout(차례인 사람이 나갔을 때), ResetGame(새 판·첫 판)에서 불림
void ABCGameModeBase::StartNextTurn()
{
	ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == false)
	{
		return;
	}

	//지금 차례 다음 사람부터 한 바퀴를 돌며 기회가 남은 플레이어를 찾는다. 기회를 다 쓴 사람과 나간 사람은 건너뛴다
	//% 연산으로 마지막 사람 다음에 다시 0번으로 돌아감 (CurrentTurnIndex가 -1이면 0번부터)
	//i는 1부터 인원 수까지 → 다음 사람부터 시작해서 마지막 반복(i == Num)에 자기 자신까지, 딱 한 바퀴만 본다
	//예) 3명, 지금 1번 차례 → (1+1)%3=2, (1+2)%3=0, (1+3)%3=1 순서로 확인
	for (int32 i = 1; i <= AllPlayerControllers.Num(); ++i)
	{
		int32 NextTurnIndex = (CurrentTurnIndex + i) % AllPlayerControllers.Num();
		ABCPlayerController* NextPlayerController = AllPlayerControllers[NextTurnIndex];
		//나간 플레이어 자리는 건너뛴다. continue: 아래 코드를 건너뛰고 다음 i로 넘어감
		//(Logout이 나간 사람 자리를 nullptr로 비워 둔다. IsValid가 걸러 준다)
		if (IsValid(NextPlayerController) == false)
		{
			continue;
		}

		ABCPlayerState* BCPS = NextPlayerController->GetPlayerState<ABCPlayerState>();
		//기회가 남은 사람을 찾으면 그 사람에게 턴을 준다
		if (IsValid(BCPS) == true && BCPS->CurrentGuessCount < BCPS->MaxGuessCount)
		{
			CurrentTurnIndex = NextTurnIndex;
			//새 턴이 시작됐으니 활동 체크를 "아직 입력 안 함"으로 되돌린다
			bHasGuessedThisTurn = false;

			//GameState에 넣으면 복제돼서 모든 클라의 타이머 위젯이 같은 이름과 같은 시간을 보여 줌
			BCGS->CurrentTurnPlayerNameString = BCPS->PlayerNameString;
			BCGS->RemainingTurnTime = TurnTimeLimit;
			//왼쪽 위 전광판의 "기회 N번 남음". 이 턴 동안은 바뀌지 않으니 턴을 시작할 때 한 번만 넣으면 됨
			BCGS->CurrentTurnRemainingGuessCount = BCPS->MaxGuessCount - BCPS->CurrentGuessCount;

			//남은 시간은 서버만 줄임. Tick 대신 1초짜리 반복 타이머라 1초에 한 번만 돎
			//SetTimer(핸들, 누구의, 어떤 함수를, 몇 초마다, 반복 여부). &ThisClass::UpdateTurnTimer = 멤버 함수의 주소
			//같은 핸들에 타이머가 이미 돌고 있으면 엔진이 그 타이머를 지우고 새로 건다 → 1초를 처음부터 다시 센다
			GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ThisClass::UpdateTurnTimer, 1.f, true);
			//턴을 줬으니 더 찾지 않고 함수를 끝냄
			return;
		}
	}
	//여기까지 왔다면 한 바퀴를 다 돌아도 기회가 남은 사람이 없었다 → 타이머를 걸지 않고 끝난다
}

void ABCGameModeBase::UpdateTurnTimer()
{
	ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == false)
	{
		return;
	}

	//RemainingTurnTime은 GameState의 Replicated 변수라 서버에서 1 줄이면 모든 클라이언트의 타이머 위젯에 반영됨
	BCGS->RemainingTurnTime--;

	//0초가 되면 입력을 못 했어도 턴을 끝냄. EndTurn에서 활동 체크로 기회를 1회 차감함
	if (BCGS->RemainingTurnTime <= 0)
	{
		EndTurn();
	}
}

//UpdateTurnTimer(시간 초과)나 PrintChatMessageString(숫자 입력)에서 불림
void ABCGameModeBase::EndTurn()
{
	//이번 턴의 1초 반복 타이머를 멈춘다. 숫자를 입력해서 일찍 끝난 턴이면 아직 돌고 있던 타이머가 여기서 멈춘다
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	//활동 체크: 이번 턴에 숫자를 입력하지 않았다 = 시간 초과. 기회를 1회 차감함
	//(숫자를 입력해서 끝난 턴은 PrintChatMessageString에서 이미 1회 늘렸으니 여기서 또 늘리지 않는다)
	if (bHasGuessedThisTurn == false)
	{
		//삼항 연산자 (조건 ? A : B): 인덱스가 유효하면 그 자리의 PC, 아니면 nullptr
		//.Get(): TObjectPtr에서 일반 포인터(ABCPlayerController*)를 꺼낸다 → 삼항 연산자의 두 결과가 둘 다 ABCPlayerController*가 됨
		ABCPlayerController* TurnPlayerController = AllPlayerControllers.IsValidIndex(CurrentTurnIndex) == true ? AllPlayerControllers[CurrentTurnIndex].Get() : nullptr;
		if (IsValid(TurnPlayerController) == true)
		{
			IncreaseGuessCount(TurnPlayerController);

			ABCPlayerState* BCPS = TurnPlayerController->GetPlayerState<ABCPlayerState>();
			if (IsValid(BCPS) == true)
			{
				BroadcastChatMessageString(BCPS->GetPlayerInfoString() + TEXT(" 시간 초과! 기회가 1회 소진되었습니다."));
			}
		}

		//시간 초과로 마지막 기회까지 다 쓴 거라면 무승부일 수 있음. 3S는 나올 수 없으니 스트라이크 0으로 판정함
		if (JudgeGame(TurnPlayerController, 0) == true)
		{
			return;
		}
	}

	//게임이 안 끝났으면 다음 사람에게 턴을 넘김
	StartNextTurn();
}

//JudgeGame에서 승리나 무승부가 나왔을 때 불림
void ABCGameModeBase::EndGame(const FString& InResultString)
{
	//턴 타이머를 멈추고 남은 시간을 0으로 만듦 → 리셋 전까지는 IsPlayerTurn이 모두 false라 누구의 숫자도 판정하지 않는다
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	ABCGameStateBase* BCGS = GetGameState<ABCGameStateBase>();
	if (IsValid(BCGS) == true)
	{
		BCGS->RemainingTurnTime = 0;
	}

	//NotificationText는 복제 변수라 서버에서 바꾸면 각 클라의 공지 위젯(WBP_NotficationText)에 뜬다
	//PC마다 같은 공지 문구를 넣어 줘야 각자 화면에 뜸
	BroadcastNotificationText(FText::FromString(InResultString));

	//공지를 바로 지우면 못 보니 ResetGameDelay초 뒤에 한 번만 ResetGame을 부름
	//마지막 인자 false = 반복하지 않음. 이 타이머가 도는 동안은 OnPostLogin이 새 턴을 시작하지 않는다 (IsTimerActive 검사)
	GetWorldTimerManager().SetTimer(ResetGameTimerHandle, this, &ThisClass::ResetGame, ResetGameDelay, false);
}
