#include "Run/Interact/SoulHighlightComponent.h"
#include "Run/Interact/SoulInteractableComponent.h"
#include "Utility/SoulLog.h"

#include "Components/MeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace SoulHighlight
{
	static const FName ColorParam(TEXT("Color"));
	static const FName IntensityParam(TEXT("Intensity"));
	static const FName FresnelParam(TEXT("FresnelExponent"));
	static const FName NoHighlightTag(TEXT("NoHighlight"));
}

USoulHighlightComponent::USoulHighlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USoulHighlightComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!Root)
		return;

	// 오버레이 대상: 보이는 메시 전부 (NoHighlight 태그는 뺀다)
	TArray<UMeshComponent*> Found;
	Owner->GetComponents<UMeshComponent>(Found);
	for (UMeshComponent* Mesh : Found)
	{
		if (Mesh && Mesh->IsVisible() && !Mesh->ComponentHasTag(SoulHighlight::NoHighlightTag))
		{
			Meshes.Add(Mesh);
		}
	}

	if (OverlayMaterial)
	{
		MID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
		MID->SetVectorParameterValue(SoulHighlight::ColorParam, Color);
		MID->SetScalarParameterValue(SoulHighlight::FresnelParam, FresnelExponent);
	}

	if (LightIntensity > 0.f)
	{
		Light = NewObject<UPointLightComponent>(Owner, TEXT("SoulHighlightLight"));
		Light->SetupAttachment(Root);
		Light->SetRelativeLocation(LightOffset);
		Light->SetCastShadows(false);
		Light->SetLightColor(Color);
		Light->SetAttenuationRadius(LightRadius);
		Light->SetIntensity(LightIntensity);
		Light->RegisterComponent();
	}

	if (bBeacon)
	{
		UStaticMesh* Mesh = BeaconMesh ? BeaconMesh.Get() : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (Mesh && OverlayMaterial)
		{
			Beacon = NewObject<UStaticMeshComponent>(Owner, TEXT("SoulHighlightBeacon"));
			Beacon->SetupAttachment(Root);
			Beacon->SetStaticMesh(Mesh);
			Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Beacon->SetCastShadow(false);
			// 원기둥 피벗이 가운데라 높이의 반만큼 올린다
			Beacon->SetRelativeScale3D(BeaconScale);
			Beacon->SetRelativeLocation(FVector(0.f, 0.f, 50.f * BeaconScale.Z));
			Beacon->ComponentTags.Add(SoulHighlight::NoHighlightTag);
			BeaconMID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
			BeaconMID->SetVectorParameterValue(SoulHighlight::ColorParam, Color);
			BeaconMID->SetScalarParameterValue(SoulHighlight::FresnelParam, BeaconFresnelExponent);
			const int32 NumMaterials = FMath::Max(Beacon->GetNumMaterials(), 1);
			for (int32 i = 0; i < NumMaterials; ++i)
			{
				Beacon->SetMaterial(i, BeaconMID);
			}
			Beacon->RegisterComponent();
		}
	}

	bActive = IsOwnerInteractable();
	ApplyOverlay(bActive);
	SW_LOG("[HIGHLIGHT] %s meshes=%d overlay=%d light=%d beacon=%d active=%d", *Owner->GetName(), Meshes.Num(), MID != nullptr, Light != nullptr, Beacon != nullptr, bActive);
}

void USoulHighlightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ApplyOverlay(false);
	Super::EndPlay(EndPlayReason);
}

bool USoulHighlightComponent::IsOwnerInteractable() const
{
	const USoulInteractableComponent* Interactable = GetOwner() ? GetOwner()->FindComponentByClass<USoulInteractableComponent>() : nullptr;
	return !Interactable || Interactable->bEnabled;
}

void USoulHighlightComponent::ApplyOverlay(bool bOn)
{
	for (const TWeakObjectPtr<UMeshComponent>& Mesh : Meshes)
	{
		if (Mesh.IsValid())
		{
			Mesh->SetOverlayMaterial(bOn ? MID.Get() : nullptr);
		}
	}
	if (Light)
	{
		Light->SetVisibility(bOn);
	}
	if (Beacon)
	{
		Beacon->SetVisibility(bOn);
	}
}

void USoulHighlightComponent::SetHighlightActive(bool bInActive)
{
	if (bActive == bInActive)
		return;
	bActive = bInActive;
	ApplyOverlay(bActive);
	SW_LOG("[HIGHLIGHT] %s active=%d", GetOwner() ? *GetOwner()->GetName() : TEXT("?"), bActive);
}

void USoulHighlightComponent::OnItemTraced()
{
	bTraced = true;
}

void USoulHighlightComponent::OffItemTraced()
{
	bTraced = false;
}

float USoulHighlightComponent::UpdateNear()
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const float Distance = Pawn ? FVector::Dist(Pawn->GetActorLocation(), GetOwner()->GetActorLocation()) : -1.f;
	bNear = NearRadius > 0.f && Distance >= 0.f && Distance <= NearRadius;
	return Distance;
}

void USoulHighlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 1회용을 쓰면(상호작용 꺼짐) 끈다. 다시 켜지면 돌아온다
	const bool bShouldBeActive = IsOwnerInteractable();
	if (bShouldBeActive != bActive)
	{
		SetHighlightActive(bShouldBeActive);
	}
	if (!bActive)
		return;

	NearCheckTimer -= DeltaTime;
	if (NearCheckTimer <= 0.f)
	{
		NearCheckTimer = 0.1f;
		UpdateNear();
	}

	Focus = FMath::FInterpTo(Focus, IsFocused() ? 1.f : 0.f, DeltaTime, BlendSpeed);
	const float Time = GetWorld()->GetTimeSeconds();
	const float Pulse = 1.f + PulseDepth * FMath::Sin(2.f * PI * PulseSpeed * Time);
	const float Base = FMath::Lerp(IdleIntensity, FocusIntensity, Focus);

	if (MID)
	{
		MID->SetScalarParameterValue(SoulHighlight::IntensityParam, Base * Pulse);
	}
	if (BeaconMID)
	{
		BeaconMID->SetScalarParameterValue(SoulHighlight::IntensityParam, IdleIntensity * BeaconIntensityScale * Pulse);
	}
	if (Light)
	{
		Light->SetIntensity(LightIntensity * FMath::Lerp(1.f, FocusLightScale, Focus) * Pulse);
	}
}
