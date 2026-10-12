#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BCAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class USoundClass;
class USoundMix;

//내 컴퓨터의 소리와 음량만 맡음. GameInstance에 붙어서 메뉴에서 게임으로 이동해도 설정과 BGM이 유지됨
UCLASS()
class BULLSANDCOWSJYJ_API UBCAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//위젯이나 PC가 자기 GameInstance의 소리 관리자를 찾을 때 씀. 서버의 규칙·복제 데이터는 건드리지 않음
	static UBCAudioSubsystem* Get(const UObject* WorldContextObject);
	static void PlayEffect(const UObject* WorldContextObject, FName SoundName);

	//데디케이티드 서버는 소리를 낼 화면이 없으므로 관리자를 만들지 않음
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//소리 설정 위젯이 생성될 때 호출함. 이미 재생 중인 BGM은 또 만들지 않음
	void StartMusic();
	void SetVolume(float InVolume);
	void ToggleMuted();
	void SaveSettings() const;
	float GetVolume() const { return Volume; }
	bool IsMuted() const { return bMuted; }

private:
	//필요한 음원만 불러와 보관함. UPROPERTY 참조가 있어야 GC가 재생용 객체를 지우지 않음
	USoundBase* LoadSound(FName SoundName);
	void ApplyVolume();

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<USoundBase>> Sounds;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	//BGM과 효과음이 같은 SoundClass를 쓰므로 슬라이더 하나로 이미 재생 중인 소리까지 조절할 수 있음
	UPROPERTY(Transient)
	TObjectPtr<USoundClass> AudioClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> VolumeMix;

	//게임 규칙과 관계없는 로컬 설정. 초기 음량은 절반이며 음소거를 풀 때 이전 음량을 그대로 씀
	float Volume = 0.5f;
	bool bMuted = false;
};
