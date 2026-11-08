#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "SoulInteractableComponent.generated.h"

class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoulInteractedSignature, APawn*, Instigator);

/**
 * F로 상호작용하는 물체 표시 (이벤트 오브젝트, 상자, 포탈, 석상, 게이트, 보드). 구 범위가 아이템 트레이스에 걸린다.
 * BP는 이 컴포넌트 하나를 얹고 On Interacted만 구현하면 된다. 프롬프트는 F 다음에 붙는 문구다.
 */
UCLASS(ClassGroup = (Soul), meta = (BlueprintSpawnableComponent))
class PROJECTSW_API USoulInteractableComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	USoulInteractableComponent();

	/** 상호작용한다. 꺼져 있으면 false */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	bool Interact(APawn* Instigator);

	/** F 다음에 보일 문구 (예: 석상에 기도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	FText PromptText;

	/** 켜져 있을 때만 프롬프트가 뜨고 F가 먹는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	bool bEnabled = true;

	/** 켜면 한 번 쓴 뒤 꺼진다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	bool bOneShot = false;

	UPROPERTY(BlueprintAssignable, Category = "Interact")
	FSoulInteractedSignature OnInteracted;
};
