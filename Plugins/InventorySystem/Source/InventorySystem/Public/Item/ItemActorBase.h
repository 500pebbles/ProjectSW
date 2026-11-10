// 

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/GameFramework/Actor.h"
#include "ItemActorBase.generated.h"

class UItemData;
class UItemComponent;
class UItemStaticMeshComponent;

UCLASS()
class INVENTORYSYSTEM_API AItemActorBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AItemActorBase();
	
	// 아이템 컴포넌트가 보유하고 있는 데이터 테이블 행 정보인 Row를 설정하는 함수
	void SetupItem(const FDataTableRowHandle& InRow);
	
	// 아이템 인스턴스의 데이터를 그대로 유지한체로 버리기 위한 함수 오버로드
	void SetupItem(UItemData* InItem);
	
public:
	UItemComponent* GetItemComponent() const { return ItemComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
private:
	// 아이템 컴포넌트의 Row를 이용해 아이템 
	void ApplyItemDataFromRow();
	
	// ApplyItemDataFromRow에서 내부적으로 수행할 로직
	void ApplyItemData(UStaticMesh* InMesh, UMaterialInterface* InMaterial);
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<UItemStaticMeshComponent> ItemMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<UItemComponent> ItemComponent;
};
