#include "UI/BCAudioSettings.h"

#include "Audio/BCAudioSubsystem.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UBCAudioSettings::NativeConstruct()
{
	Super::NativeConstruct();
	Panel_SoundOptions->SetVisibility(ESlateVisibility::Collapsed);
	if (UBCAudioSubsystem* Audio = UBCAudioSubsystem::Get(this))
	{
		//새 화면에서도 관리자의 현재 설정을 그대로 표시함. BGM이 이미 있으면 StartMusic은 새 음악을 만들지 않음
		Audio->StartMusic();
		Slider_Volume->SetValue(Audio->GetVolume());
		TextBlock_MuteLabel->SetText(FText::FromString(Audio->IsMuted() ? TEXT("음소거 해제") : TEXT("음소거 켜기")));
	}
	Button_Sound->OnClicked.AddUniqueDynamic(this, &ThisClass::OnSoundButtonClicked);
	Button_Mute->OnClicked.AddUniqueDynamic(this, &ThisClass::OnMuteButtonClicked);
	Slider_Volume->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnVolumeChanged);
	//마우스와 게임패드 모두 조절을 끝냈을 때 저장함
	Slider_Volume->OnMouseCaptureEnd.AddUniqueDynamic(this, &ThisClass::OnVolumeCommitted);
	Slider_Volume->OnControllerCaptureEnd.AddUniqueDynamic(this, &ThisClass::OnVolumeCommitted);
}

void UBCAudioSettings::NativeDestruct()
{
	//키보드로 음량을 바꾼 뒤 레벨을 옮기는 경우에도 마지막 설정을 남김
	OnVolumeCommitted();
	Super::NativeDestruct();
}

void UBCAudioSettings::OnSoundButtonClicked()
{
	UBCAudioSubsystem::PlayEffect(this, TEXT("S_Click"));
	//판은 필요할 때만 켬. 판 자체는 클릭을 막지 않고 안의 슬라이더·버튼만 클릭을 받음
	Panel_SoundOptions->SetVisibility(Panel_SoundOptions->IsVisible() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void UBCAudioSettings::OnMuteButtonClicked()
{
	if (UBCAudioSubsystem* Audio = UBCAudioSubsystem::Get(this))
	{
		Audio->ToggleMuted();
		TextBlock_MuteLabel->SetText(FText::FromString(Audio->IsMuted() ? TEXT("음소거 해제") : TEXT("음소거 켜기")));
		//음소거를 켰을 때는 클릭음도 끔. 해제했을 때는 클릭음으로 돌아온 소리를 알려 줌
		UBCAudioSubsystem::PlayEffect(this, TEXT("S_Click"));
	}
}

void UBCAudioSettings::OnVolumeChanged(float InVolume)
{
	if (UBCAudioSubsystem* Audio = UBCAudioSubsystem::Get(this))
	{
		Audio->SetVolume(InVolume);
	}
}

void UBCAudioSettings::OnVolumeCommitted()
{
	if (UBCAudioSubsystem* Audio = UBCAudioSubsystem::Get(this))
	{
		Audio->SaveSettings();
	}
}
