
#include "SoulCharacterPlayer.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/SoulPlayerController.h"
#include "Player/SoulPlayerState.h"
#include "UI/HUD/SoulHUD.h"

ASoulCharacterPlayer::ASoulCharacterPlayer()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SpringArm->SetupAttachment(RootComponent);
	Camera->SetupAttachment(SpringArm);
}

void ASoulCharacterPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilityActorInfo();
}

void ASoulCharacterPlayer::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitAbilityActorInfo();
}

void ASoulCharacterPlayer::InitAbilityActorInfo()
{
	// Step 1 : InitAbilityActorInfo
	ASoulPlayerState* SoulPlayerState = GetPlayerState<ASoulPlayerState>();
	if (!IsValid(SoulPlayerState)) return;
	
	SoulPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(SoulPlayerState, this);
	AbilitySystemComponent = SoulPlayerState->GetAbilitySystemComponent();
	AttributeSet = SoulPlayerState->GetAttributeSet();
	
	// Step 2 : InitOverlay 
	ASoulPlayerController* SoulPlayerController = Cast<ASoulPlayerController>(GetController());
	if (!IsValid(SoulPlayerController)) return;
	ASoulHUD* SoulHUD = Cast<ASoulHUD>(SoulPlayerController->GetHUD());
	if (!IsValid(SoulHUD)) return;
	
	SoulHUD->InitOverlay(SoulPlayerController, SoulPlayerState, AbilitySystemComponent, AttributeSet);	
}