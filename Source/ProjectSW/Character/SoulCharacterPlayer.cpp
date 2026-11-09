
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

void ASoulCharacterPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	int32 Level = GetCharacterLevel();
	UE_LOG(LogTemp, Log, TEXT("LEVEL: %d"), Level);
}

void ASoulCharacterPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilityActorInfo();
	GiveStartupAbilities();
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
	
	// Step 1 : InitAbilityActorInfo 호출, Hook 실행으로 GameplayEffect 적용에 대한 델리게이트 바인딩. 
	SoulPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(SoulPlayerState, this);
	Cast<USoulAbilitySystemComponent>(SoulPlayerState->GetAbilitySystemComponent())->OnAbilityActorInfoSet();
	
	// Step 2 : PlayerState가 소유한 ASC/AS 주소를 참조. 
	AbilitySystemComponent = SoulPlayerState->GetAbilitySystemComponent();
	AttributeSet = SoulPlayerState->GetAttributeSet();
	
	// Step 3 : Initialize Attributes.
	InitializeDefaultAttributes();
	
	// Step 4 : InitOverlay on HUD. (여기부터 자신의 로컬 클라이언트에서만 호출)
	ASoulPlayerController* SoulPlayerController = Cast<ASoulPlayerController>(GetController());
	if (!IsValid(SoulPlayerController)) return;
	ASoulHUD* SoulHUD = Cast<ASoulHUD>(SoulPlayerController->GetHUD());
	if (!IsValid(SoulHUD)) return;
	
	SoulHUD->InitOverlay(SoulPlayerController, SoulPlayerState, AbilitySystemComponent, AttributeSet);	
	SoulHUD->InitAttributeMenu(SoulPlayerController, SoulPlayerState, AbilitySystemComponent, AttributeSet);
}

int32 ASoulCharacterPlayer::GetCharacterLevel()
{
	const ASoulPlayerState* SoulPlayerState = GetPlayerState<ASoulPlayerState>();
	if (!IsValid(SoulPlayerState)) return 1;
	
	return SoulPlayerState->GetPlayerLevel();
}