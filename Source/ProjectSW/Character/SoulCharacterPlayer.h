
#pragma once

#include "CoreMinimal.h"
#include "SoulCharacterBase.h"
#include "SoulCharacterPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UCharacterEquipmentComponent;

UCLASS()
class PROJECTSW_API ASoulCharacterPlayer : public ASoulCharacterBase
{
	GENERATED_BODY()
	
public:
	ASoulCharacterPlayer();
	
	virtual void BeginPlay() override;
	
public:
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;

	/** 순간이동 뒤 의상 물리(KawaiiPhysics 등 애님 동역학)를 새 자리 자세로 다시 잡는다 */
	virtual bool TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest = false, bool bNoCheck = false) override;

/* GAS Section */
protected:
	virtual void InitAbilityActorInfo() override;
	

/* Combat Interface */
public:
	virtual int32 GetCharacterLevel() override;	
	
/* Component Section */
protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;
	
	// 장비 착용 레벨 제한(플레이어 레벨과 비교). 인벤토리의 InitializeComponent가 이 컴포넌트를 찾으므로
	// 인벤토리보다 먼저 있도록 생성자에서 만든다. 없으면 인벤토리가 레벨 제한 없는 기본 UEquipmentComponent를 붙인다
	UPROPERTY(VisibleAnywhere, Category = "Equipment")
	TObjectPtr<UCharacterEquipmentComponent> CharacterEquipment;
	
};
