#include "LinearFinishLine.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "STR_RacerPawn.h"
#include "RaceGameMode.h"

// Constructor
ALinearFinishLine::ALinearFinishLine()
{
	PrimaryActorTick.bCanEverTick = false;

	// Collision box
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);

	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);

	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);

	// Direction arrow
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
	ArrowComponent->ArrowSize = 2.0f;
}

// BeginPlay
void ALinearFinishLine::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ALinearFinishLine::OnOverlapBegin);
}

// Overlap logic
void ALinearFinishLine::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || !OtherComp) return;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	AController* Controller = Pawn->GetController();
	if (!Controller) return;

	ARaceGameMode* GameMode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GameMode) return;

	// Déjà fini ?
	if (GameMode->IsControllerFinished(Controller)) return;

	// Vitesse
	float Speed = Pawn->GetVelocity().Size();
	FVector MoveDir = Pawn->GetVelocity().GetSafeNormal();

	if (ASTR_RacerPawn* STR = Cast<ASTR_RacerPawn>(Pawn))
	{
		Speed = STR->GetCurrentSpeed();
		MoveDir = STR->GetActorForwardVector();
	}

	if (Speed < MinSpeed) return;

	// Direction
	const FVector ForwardVector = ArrowComponent->GetForwardVector();
	const float Dot = FVector::DotProduct(MoveDir, ForwardVector);

	if (Dot < MinForwardDot) return;

	// Anti double trigger
	if (AlreadyTriggered.Contains(OtherActor)) return;

	AlreadyTriggered.Add(OtherActor);

	FTimerHandle Tmp;
	GetWorldTimerManager().SetTimer(Tmp, [this, OtherActor]()
		{
			AlreadyTriggered.Remove(OtherActor);
		}, 0.5f, false);

	// FIN DE COURSE
	GameMode->NotifyPlayerFinished(Pawn);
}