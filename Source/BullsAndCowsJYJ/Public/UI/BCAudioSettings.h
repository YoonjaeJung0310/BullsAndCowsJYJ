#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BCAudioSettings.generated.h"

class UButton;
class USlider;
class UTextBlock;

//메뉴와 게임 화면에서 함께 쓰는 작은 소리 설정 위젯. 음량 상태는 이 위젯이 아니라 GameInstance의 관리자가 갖고 있음
UCLASS()
class BULLSANDCOWSJYJ_API UBCAudioSettings : public UUserWidget
{
	GENERATED_BODY()

protected:
	//BindWidget 이름과 타입은 WBP_AudioSettings의 자식 위젯과 맞아야 함
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Sound;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Mute;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USlider> Slider_Volume;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_MuteLabel;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> Panel_SoundOptions;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	//버튼·슬라이더의 다이내믹 델리게이트에 연결하므로 UFUNCTION으로 등록함
	UFUNCTION()
	void OnSoundButtonClicked();
	UFUNCTION()
	void OnMuteButtonClicked();
	UFUNCTION()
	void OnVolumeChanged(float InVolume);
	UFUNCTION()
	void OnVolumeCommitted();
};
