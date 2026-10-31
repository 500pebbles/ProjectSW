// 

#include "Item/ItemActorBase.h"

#include "Item/ItemComponent.h"
#include "Item/ItemStaticMeshComponent.h"
#include "Item/Data/ItemData.h"

struct FItemDataTable;
// Sets default values
AItemActorBase::AItemActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ItemMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMeshComponent->SetupAttachment(SceneRoot);

	ItemComponent = CreateDefaultSubobject<UItemComponent>(TEXT("ItemComponent"));
}

void AItemActorBase::SetupItem(const FDataTableRowHandle& InRow)
{
	if (ItemComponent)
	{
		// 아이템 행만 설정하면 이후 BeginPlay에서 알아서 읽어서 초기화함
		ItemComponent->ItemRow = InRow;
	}
}

// Called when the game starts or when spawned
void AItemActorBase::BeginPlay()
{
	Super::BeginPlay();
	
	// 레벨에 이미 배치되있는 경우에는 에디터에서 Row를 설정했기 때문에 바로 실행 가능
	ApplyItemDataFromRow();
}

// 아이템 컴포넌트에 저장된 행을 이용해 아이템 데이터를 받아와 초기화하는 함수
void AItemActorBase::ApplyItemDataFromRow()
{
	if (!ItemComponent || ItemComponent->ItemRow.IsNull())
	{
		return;
	}

	const FItemDataTable* Row = ItemComponent->ItemRow.GetRow<FItemDataTable>(TEXT("Can't Find Row, Function: AItemActor::ApplyWorldMeshFromRow"));
	if (!Row)
	{
		return;
	}

	ApplyItemData(Row->ItemMesh.LoadSynchronous(), Row->ItemMaterialInstance.LoadSynchronous());
}

// 아이템 데이터 내부의 값들을 이용해 초기화하는 함수
void AItemActorBase::ApplyItemData(UStaticMesh* InMesh, UMaterialInterface* InMaterial)
{
	if (!InMesh || !ItemMeshComponent)
	{
		return;
	}

	ItemMeshComponent->SetStaticMesh(InMesh);
	if (InMaterial)
	{
		ItemMeshComponent->SetMaterial(0, InMaterial);
	}

	// 메시 로컬 공간의 바운딩 박스 (회전/스케일은 적용하지 않음)
	const FBox LocalBox = InMesh->GetBounds().GetBox();

	// X/Y는 박스 중앙을 루트에 맞추고, Z는 박스 바닥을 루트에 맞춘다.
	const FVector Center = LocalBox.GetCenter();
	const FVector AlignOffset(-Center.X, -Center.Y, -LocalBox.Min.Z);

	// 자동 정렬 결과에 데이터 테이블의 수동 보정값을 더한다.
	ItemMeshComponent->SetRelativeLocation(AlignOffset);
}
