
#include "SoulPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "Input/SoulEnhancedInputComponent.h"
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
	EnhancedInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
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
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ASoulPlayerController::AbilityInputTagPressed(const FGameplayTag InputTag)
{
	// 입력 정책 : 누른 순간 1회 발동, 실행 중이면 입력만 전달 (콤보 입력). Held 연사 없음.
	if (!IsValid(GetASC())) return;
	GetASC()->AbilityInputTagPressed(InputTag);
}

void ASoulPlayerController::AbilityInputTagReleased(const FGameplayTag InputTag)
{
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

void ASoulPlayerController::ShowDamageText_Implementation(float DamageAmount, ACharacter* TargetCharacter, bool bIsCriticalHit)
{
	if (!IsValid(TargetCharacter)) return;
	
	USoulDamageTextComponent* DamageText = NewObject<USoulDamageTextComponent>(TargetCharacter, DamageTextComponentClass);
	DamageText->RegisterComponent();
	DamageText->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	DamageText->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	DamageText->SetDamageText(DamageAmount, bIsCriticalHit);
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