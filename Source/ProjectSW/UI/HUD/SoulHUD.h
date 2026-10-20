// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SoulHUD.generated.h"


class UAttributeSet;
class UAbilitySystemComponent;
struct FWidgetControllerParams;
class UOverlayWidgetController;
class USoulUserWidget;

UCLASS()
class PROJECTSW_API ASoulHUD : public AHUD
{
	GENERATED_BODY()
	
/* Overlay Widget Section */	
public:
	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
protected:
	UPROPERTY()
	TObjectPtr<USoulUserWidget> OverlayWidget;
	
	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<USoulUserWidget> OverlayWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;	
};
