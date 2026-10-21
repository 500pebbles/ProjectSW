
#include "SoulCharacterPlayer.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
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
	ASoulPlayerState* SoulPlayerState = GetPlayerState<ASoulPlayerState>();
	check(SoulPlayerState);
	
	// Step 1 : InitAbilityActorInfo 호출, Hook 실행으로 GameplayEffect Apply 델리게이트 바인딩. 
	SoulPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(SoulPlayerState, this);
	Cast<USoulAbilitySystemComponent>(SoulPlayerState->GetAbilitySystemComponent())->OnAbilityActorInfoSet();
	
	// Step 2 : PlayerState가 소유한 ASC/AS 주소를 참조. 
	AbilitySystemComponent = SoulPlayerState->GetAbilitySystemComponent();
	AttributeSet = SoulPlayerState->GetAttributeSet();
	
	// Step 3 : InitOverlay 
	ASoulPlayerController* SoulPlayerController = Cast<ASoulPlayerController>(GetController());
	if (!IsValid(SoulPlayerController)) return;
	ASoulHUD* SoulHUD = Cast<ASoulHUD>(SoulPlayerController->GetHUD());
	if (!IsValid(SoulHUD)) return;
	
	SoulHUD->InitOverlay(SoulPlayerController, SoulPlayerState, AbilitySystemComponent, AttributeSet);
	
	// Step 4 : Initialize Attributes.
	InitializeDefaultAttributes();
}

int32 ASoulCharacterPlayer::GetPlayerLevel()
{
	const ASoulPlayerState* SoulPlayerState = GetPlayerState<ASoulPlayerState>();
	if (!IsValid(SoulPlayerState)) return 1;
	
	return SoulPlayerState->GetPlayerLevel();
}
