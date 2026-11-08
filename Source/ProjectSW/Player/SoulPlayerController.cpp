
#include "SoulPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "Input/SoulEnhancedInputComponent.h"
#include "UI/HUD/SoulHUD.h"
#include "Map/SoulMapWidget.h"
#include "Map/SoulMapSubsystem.h"
#include "UI/WidgetComponent/SoulDamageTextComponent.h"
#include "Utility/SoulGameplayTags.h"
#include "Utility/SoulDefineMacro.h"
#include "Utility/SoulLog.h"


ASoulPlayerController::ASoulPlayerController()
{
	bReplicates = true;
}

void ASoulPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ASoulPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem)) return;

	for (UInputMappingContext* Context : InputMappingContexts)
	{
		InputSubsystem->AddMappingContext(Context, 0);
	}

	USoulEnhancedInputComponent* EnhancedInputComponent = Cast<USoulEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;
	
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::StopMove);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);
	EnhancedInputComponent->BindAction(HoverMouseAction, ETriggerEvent::Started, this, &ThisClass::HoverMouse);
	EnhancedInputComponent->BindAction(HoverMouseAction, ETriggerEvent::Completed, this, &ThisClass::StopHoverMouse);
	EnhancedInputComponent->BindAction(TabAction, ETriggerEvent::Started, this, &ThisClass::ShowStatus);
	EnhancedInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);

	if (MapAction)
		EnhancedInputComponent->BindAction(MapAction, ETriggerEvent::Started, this, &ThisClass::ToggleMap);
}

void ASoulPlayerController::Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;

	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	const FVector2D MovementVector = Value.Get<FVector2D>();
	
	// 지금 누르고 있는 이동 방향. 공격 중에도 기록 (회피 방향용).
	MoveInputDirection = (ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X).GetSafeNormal();
	
	// 공격 중에는 이동하지 않고 입력 방향만 기록. 공격 GA가 다음 타 시작 시 이 방향으로 회전.
	if (IsValid(GetASC()) && GetASC()->HasMatchingGameplayTag(SoulTags::States::Attacking))
	{
		AttackInputDirection = MoveInputDirection;
		return;
	}
	
	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);

}

void ASoulPlayerController::StopMove()
{
	MoveInputDirection = FVector::ZeroVector;
}

void ASoulPlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void ASoulPlayerController::Jump()
{
	if (!IsValid(GetCharacter())) return;

	GetCharacter()->Jump();
}

void ASoulPlayerController::ShowStatus()
{
	ASoulHUD* SoulHUD = Cast<ASoulHUD>(GetHUD());	
	if (!IsValid(SoulHUD)) return;
	SoulHUD->ToggleAttributeMenuWidget();
}

void ASoulPlayerController::StopJumping()
{
	if (!IsValid(GetCharacter())) return;

	GetCharacter()->StopJumping();
}

void ASoulPlayerController::HoverMouse()
{
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ASoulPlayerController::StopHoverMouse()
{   
	// 임시로 지도가 열려 있으면 입력 모드는 ToggleMap이 관리한다.
	const ASoulHUD* SoulHUD = Cast<ASoulHUD>(GetHUD());
	if (IsValid(SoulHUD) && SoulHUD->IsMapOpen()) return;

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ASoulPlayerController::AbilityInputTagPressed(const FGameplayTag InputTag)
{
	// 대시 키를 누르면 질주 속도로 변경 (0.25초 대시 종료 후에도 키를 누르고 있으면 빠른 속도로 계속 달림)
	if (InputTag.MatchesTagExact(SoulTags::InputTags::Dash))
	{
		bSprintHeld = true;
		RefreshMoveSpeed();
	}
	
	// 입력 정책 : 누른 순간 1회 발동, 실행 중이면 입력만 전달 (콤보 입력). Held 연사 없음.
	if (!IsValid(GetASC())) return;
	GetASC()->AbilityInputTagPressed(InputTag);
}

void ASoulPlayerController::AbilityInputTagReleased(const FGameplayTag InputTag)
{
	// 대시 키에서 손을 떼면 기본 이동 속도로 복구
	if (InputTag.MatchesTagExact(SoulTags::InputTags::Dash))
	{
		bSprintHeld = false;
		RefreshMoveSpeed();
	}
	
	if (!IsValid(GetASC())) return;
	GetASC()->AbilityInputTagReleased(InputTag);
}

void ASoulPlayerController::AbilityInputTagHeld(const FGameplayTag InputTag)
{
	if (!IsValid(GetASC())) return;
	GetASC()->AbilityInputTagHeld(InputTag);
}

USoulAbilitySystemComponent* ASoulPlayerController::GetASC()
{
	if (SoulAbilitySystemComponent == nullptr)
		SoulAbilitySystemComponent = Cast<USoulAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));

	return SoulAbilitySystemComponent;
}

void ASoulPlayerController::ToggleMap()
{
	ASoulHUD* SoulHUD = Cast<ASoulHUD>(GetHUD());
	
	if (!IsValid(SoulHUD))
		return;

	if (SoulHUD->IsMapOpen())
	{
		SoulHUD->CloseMap();
		FlushPressedKeys();
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		return;
	}

	const USoulMapSubsystem* MapSubsystem = GetWorld()->GetSubsystem<USoulMapSubsystem>();

	if (!MapSubsystem || !MapSubsystem->HasMapData())
		return;

	USoulMapWidget* MapWidget = SoulHUD->OpenMap();
	if (!IsValid(MapWidget))
		return;

	FlushPressedKeys();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MapWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ASoulPlayerController::ShowDamageText_Implementation(float DamageAmount, ACharacter* TargetCharacter, bool bIsCriticalHit, int32 DamageType, bool bStackAbove)
{
	if (!IsValid(TargetCharacter) && !IsLocalController()) return;
	
	USoulDamageTextComponent* DamageText = NewObject<USoulDamageTextComponent>(TargetCharacter, DamageTextComponentClass);
	DamageText->RegisterComponent();
	DamageText->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	DamageText->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	if (bStackAbove) DamageText->AddWorldOffset(ElementTextOffset * (bIsCriticalHit ? ElementTextCritScale : 1.f));
	DamageText->SetDamageText(DamageAmount, bIsCriticalHit, DamageType);
}

void ASoulPlayerController::SetMoveSpeedMultiplier(float Multiplier)
{
	MoveSpeedMultiplier = Multiplier;
	RefreshMoveSpeed();
}

void ASoulPlayerController::RefreshMoveSpeed()
{
	// 대시 키 상태에 따라 걷기 또는 질주 속도에 의상 배율을 곱한다
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		if (UCharacterMovementComponent* MoveComp = ControlledCharacter->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = (bSprintHeld ? SprintSpeed : WalkSpeed) * MoveSpeedMultiplier;
		}
	}
}

FVector ASoulPlayerController::ConsumeAttackInputDirection()
{
	const FVector Direction = AttackInputDirection;
	AttackInputDirection = FVector::ZeroVector;
	return Direction;
}

FVector ASoulPlayerController::GetMoveInputDirection() const
{
	return MoveInputDirection;
}