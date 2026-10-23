// 

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoulAbilitySystemLibrary.generated.h"


class UAttributeMenuWidgetController;
class UOverlayWidgetController;

/* 리플렉션 시스템을 기반으로 함수를 전역에 등록하는 BlueprintFunctionLibrary. 
 * 해당 클래스 내부의 정적 멤버함수들은 UObject를 베이스로 생성한 블루프린트에서 정적 호출이 가능.
 * 라이브러리 인스턴스가 없어 WorldContext를 파라미터로 제공받아 게임 월드에 접근함. */

UCLASS()
class PROJECTSW_API USoulAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "SoulAbilitySystemLibrary")
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category="SoulAbilitySystemLibrary")
	static UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);
private:
};