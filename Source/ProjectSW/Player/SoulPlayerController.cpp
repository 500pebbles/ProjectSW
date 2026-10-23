
#include "SoulPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "Input/SoulEnhancedInputComponent.h"
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
	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);

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
	// 키를 누를때 말고 뗄떼 어빌리티 실행? 전투 파트담당이 결정.
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