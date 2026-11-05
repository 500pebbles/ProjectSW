// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SoulHUD.generated.h"


class UAttributeMenuWidgetController;
class UAttributeSet;
class UAbilitySystemComponent;
struct FWidgetControllerParams;
class UOverlayWidgetController;
class UMapWidgetController;
class USoulUserWidget;
class USoulMapWidget;

UCLASS()
class PROJECTSW_API ASoulHUD : public AHUD
{
	GENERATED_BODY()
	
/* Overlay Widget Section */	
public:
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UMapWidgetController* GetMapWidgetController();

	USoulMapWidget* OpenMap();
	void CloseMap();
	bool IsMapOpen() const;
		
protected:
	UPROPERTY()
	TObjectPtr<USoulUserWidget> OverlayWidget;
	
	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY()
	TObjectPtr<UMapWidgetController> MapWidgetController;

	UPROPERTY()
	TObjectPtr<USoulMapWidget> MapWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<USoulUserWidget> OverlayWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;	

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<USoulMapWidget> MapWidgetClass;

	static constexpr int32 MapWidgetZOrder = 10;

/* Attribute Menu Section */	
public:
	UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams);
	
protected:
	UPROPERTY()
	TObjectPtr<UAttributeMenuWidgetController> AttributeMenuWidgetController;
	
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UAttributeMenuWidgetController> AttributeMenuWidgetControllerClass;
};