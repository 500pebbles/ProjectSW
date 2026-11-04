
#include "SoulCharacterEnemy.h"

#include "AI/SoulAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Core/SoulAbilitySystemComponent.h"
#include "GAS/Core/SoulAttributeSet.h"
#include "GAS/Utility/SoulAbilitySystemLibrary.h"
#include "UI/Widget/SoulUserWidget.h"
#include "Utility/SoulDefineMacro.h"
#include "Utility/SoulGameplayTags.h"


ASoulCharacterEnemy::ASoulCharacterEnemy()
{
	// Component Setup. 
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	
	AbilitySystemComponent = CreateDefaultSubobject<USoulAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<USoulAttributeSet>("AttributeSet");
	
	HealthBar = CreateDefaultSubobject<UWidgetComponent>("HealthBar");
	HealthBar->SetupAttachment(GetRootComponent());
	
	// Movement Setup.
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
}

void ASoulCharacterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	InitAbilityActorInfo();
	
	if (HasAuthority()) USoulAbilitySystemLibrary::EnemyGiveStartupAbilities(this, AbilitySystemComponent, EnemyType);
	
	// Setup WidgetController. 
	USoulUserWidget* SoulUserWidget = Cast<USoulUserWidget>(HealthBar->GetUserWidgetObject());
	if (SoulUserWidget) SoulUserWidget->SetWidgetController(this);		
	
	// Attribute 변경에 따른 감지 & 콜백 바인딩
	const USoulAttributeSet* AS = CastChecked<USoulAttributeSet>(AttributeSet);
	if (!IsValid(AS)) return;	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data){ OnHealthChanged.Broadcast(Data.NewValue); });
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data){ OnMaxHealthChanged.Broadcast(Data.NewValue); });
	
	// GameplayTag 변경에 따른 감지 & 콜백 바인딩. 
	AbilitySystemComponent->RegisterGameplayTagEvent(SoulTags::Combats::HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnHitReactTagChanged);
	
	// Broadcast Initial Values.
	OnHealthChanged.Broadcast(AS->GetHealth());
	OnMaxHealthChanged.Broadcast(AS->GetMaxHealth());
}

void ASoulCharacterEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!HasAuthority()) return;
	
	AIController = Cast<ASoulAIController>(NewController);
	AIController->GetBlackboardComponent()->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
	AIController->RunBehaviorTree(BehaviorTree);
	AIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), false);
	AIController->GetBlackboardComponent()->SetValueAsBool(FName("IsRangedAttacker"), EnemyType == EEnemyType::EnemyType_03 || EnemyType == EEnemyType::EnemyType_04);
}

void ASoulCharacterEnemy::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<USoulAbilitySystemComponent>(AbilitySystemComponent)->OnAbilityActorInfoSet();	
	
	if (HasAuthority()) InitializeDefaultAttributes(); 	
}

void ASoulCharacterEnemy::InitializeDefaultAttributes() const
{
	
	USoulAbilitySystemLibrary::EnemyInitializeDefaultAttributes(this, EnemyType, Level, AbilitySystemComponent);
}

int32 ASoulCharacterEnemy::GetCharacterLevel()
{
	return Level;
}

void ASoulCharacterEnemy::Die()
{
	Super::Die();
	
	UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(AIController->BrainComponent);
	if (!IsValid(BTComponent)) return;	
	BTComponent->StopTree();
	
	SetLifeSpan(10.f);	
}

void ASoulCharacterEnemy::SetCombatTarget_Implementation(AActor* CombatTargetActor)
{
	CombatTarget = CombatTargetActor;
}

AActor* ASoulCharacterEnemy::GetCombatTarget_Implementation() const
{
	return CombatTarget;
}

void ASoulCharacterEnemy::OnHitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	/* HitReact 전체 플로우  
	 * AttributeSet에서 데미지 처리의 bFatal 분기에서, CommonAbility 분류의 GA_HitReact를 임시 Tag로 실행.
	 * GA_HitReact는 GE_HitReact를 Target에 적용시키며 인터페이스로 접근, Target이 참조하는 HitReact 몽탄주를 참조해 재생.
	 * GE_HitReact는 단순한 태그 적용의 역할, GrantTags로 SoulTags::Combats::HitReact를 적용.
	 * SoulTags::Combats::HitReact 태그의 변경을 RegisterGameplayTagEvent로 감지. */
	
	bHitReacting = NewCount > 0;
	
	GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.f : BaseWalkSpeed;
	if (!IsValid(AIController)) return;
	AIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), bHitReacting);
}
