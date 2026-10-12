#include "Audio/BCAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UBCAudioSubsystem* UBCAudioSubsystem::Get(const UObject* WorldContextObject)
{
	//위젯과 PC가 서로 소리 상태를 복사하지 않고 GameInstance의 같은 관리자를 쓰게 함
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UBCAudioSubsystem>() : nullptr;
}

bool UBCAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UGameInstance* GameInstance = Cast<UGameInstance>(Outer);
	return Super::ShouldCreateSubsystem(Outer) && IsValid(GameInstance) && GameInstance->IsDedicatedServerInstance() == false;
}

void UBCAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	//Saved의 사용자 설정을 읽음. 서버로 보내거나 프로젝트 기본 설정에 음량을 덮어쓰지 않음
	GConfig->GetFloat(TEXT("BCAudio"), TEXT("Volume"), Volume, GGameUserSettingsIni);
	GConfig->GetBool(TEXT("BCAudio"), TEXT("Muted"), bMuted, GGameUserSettingsIni);
	Volume = FMath::Clamp(Volume, 0.0f, 1.0f);
	AudioClass = LoadObject<USoundClass>(nullptr, TEXT("/Game/BullsAndCowsJYJ/Audio/SC_BCAudio.SC_BCAudio"));
	VolumeMix = LoadObject<USoundMix>(nullptr, TEXT("/Game/BullsAndCowsJYJ/Audio/SM_BCVolume.SM_BCVolume"));
}

void UBCAudioSubsystem::Deinitialize()
{
	//GameInstance가 종료될 때 반복 음악도 끝냄. 메뉴 이동 때는 GameInstance가 살아 있으므로 여기로 오지 않음
	if (IsValid(MusicComponent))
	{
		MusicComponent->Stop();
		MusicComponent->DestroyComponent();
	}
	Super::Deinitialize();
}

USoundBase* UBCAudioSubsystem::LoadSound(FName SoundName)
{
	if (const TObjectPtr<USoundBase>* FoundSound = Sounds.Find(SoundName))
	{
		return FoundSound->Get();
	}
	//파일 이름은 호출부에서 넘기는 고정 이름만 씀. 음원 폴더는 패키징 설정에도 등록해 둠
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/BullsAndCowsJYJ/Audio/%s.%s"), *SoundName.ToString(), *SoundName.ToString()));
	Sounds.Add(SoundName, Sound);
	if (IsValid(Sound) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("음원 없음: %s"), *SoundName.ToString());
	}
	return Sound;
}

void UBCAudioSubsystem::ApplyVolume()
{
	if (IsValid(AudioClass) && IsValid(VolumeMix) && IsValid(GetWorld()))
	{
		//같은 기본 Mix를 다시 지정해도 중복 음악이 생기지 않음. 음소거는 설정 음량을 지우지 않고 실제 출력만 0으로 바꿈
		UGameplayStatics::SetBaseSoundMix(this, VolumeMix);
		UGameplayStatics::SetSoundMixClassOverride(this, VolumeMix, AudioClass, bMuted ? 0.0f : Volume, 1.0f, 0.05f, true);
	}
}

void UBCAudioSubsystem::StartMusic()
{
	ApplyVolume();
	if (IsValid(MusicComponent) && MusicComponent->IsPlaying())
	{
		return;
	}
	if (USoundBase* Music = LoadSound(TEXT("S_BGM")))
	{
		//레벨을 옮겨도 재생을 이어 가고, 자동 파괴를 꺼 관리자가 종료할 때 직접 정리함
		MusicComponent = UGameplayStatics::CreateSound2D(this, Music, 1.0f, 1.0f, 0.0f, nullptr, true, false);
		if (IsValid(MusicComponent))
		{
			MusicComponent->Play();
		}
	}
}

void UBCAudioSubsystem::PlayEffect(const UObject* WorldContextObject, FName SoundName)
{
	if (UBCAudioSubsystem* Audio = Get(WorldContextObject))
	{
		if (Audio->bMuted || Audio->Volume <= 0.0f)
		{
			return;
		}
		if (USoundBase* Sound = Audio->LoadSound(SoundName))
		{
			//2D 효과음이라 화면에서 같은 크기로 들림. RPC로 소리 자체를 복제하지 않고 받은 사건을 내 화면에서 재생함
			UGameplayStatics::PlaySound2D(WorldContextObject, Sound);
		}
	}
}

void UBCAudioSubsystem::SetVolume(float InVolume)
{
	Volume = FMath::Clamp(InVolume, 0.0f, 1.0f);
	ApplyVolume();
}

void UBCAudioSubsystem::ToggleMuted()
{
	bMuted = !bMuted;
	ApplyVolume();
	SaveSettings();
}

void UBCAudioSubsystem::SaveSettings() const
{
	//슬라이더를 놓거나 음소거를 바꿀 때만 저장함. 드래그 중 매 프레임 디스크에 쓰지는 않음
	GConfig->SetFloat(TEXT("BCAudio"), TEXT("Volume"), Volume, GGameUserSettingsIni);
	GConfig->SetBool(TEXT("BCAudio"), TEXT("Muted"), bMuted, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}
