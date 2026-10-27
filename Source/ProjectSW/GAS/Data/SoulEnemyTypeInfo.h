// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulEnemyTypeInfo.generated.h"

class UGameplayEffect;

/* Enemy의 Initial Attributes, Startup Abilities의 초기화를 진행하기 위한 데이터 클래스.
 * 
 * 데이터 설정
 * 1. EEnemyType 으로 Enemy의 종류를 지정한다.
 * 2. FEnemyDefaultInfo 구조체에 DefaultGE를 비롯한 '개별 설정'이 필요한 값들을 Setup으로 참조한다.
 * 3. EnemyTypeInformation 자료구조에 EnemyType과 EnemyDefaultInfo 구조체를 매핑한다.
 * 4. 해당 클래스의 에셋을 만들고, EnemyTypeInformation에 데이터를 채워넣는다.
 * 
 * 초기화
 * 1. 데이터 에셋을 쓰기 위해 어딘가에서 에셋의 인스턴스를 준비한다. 전역 접근이 가장 쉬운 GameMode 선택.
 * 2. 실제로 데이터를 기반으로 초기화의 정의는 블루프린트 라이브러리 EnemyInitializeDefaultAttributes로 정의한다.
 * 3. 게임모드에서 데이터를 채워넣은 에셋의 인스턴스를 기반, 함수의 파라미터로 들어온 타입을 기반으로 매핑된 GE 주입.  */

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	EnemyType_01,  // 표준
	EnemyType_02,  // 딜러
	EnemyType_03,  // 탱커
	EnemyType_04   // 잡몹 
};

USTRUCT(Blueprintable)
struct FEnemyDefaultInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> DefaultAttributes;
};

UCLASS()
class PROJECTSW_API USoulEnemyTypeInfo : public UDataAsset
{
	GENERATED_BODY()
	
public:
	FORCEINLINE FEnemyDefaultInfo GetEnemyDefaultInfo(EEnemyType CharacterType) const { return EnemyTypeInformation.FindChecked(CharacterType); }
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TMap<EEnemyType, FEnemyDefaultInfo> EnemyTypeInformation;	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> InitializeAttributes;		
};