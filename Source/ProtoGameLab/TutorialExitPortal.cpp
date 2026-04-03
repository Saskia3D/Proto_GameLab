#include "TutorialExitPortal.h"

#include "TutorialManager.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ATutorialExitPortal::ATutorialExitPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetBoxExtent(FVector(140.f, 140.f, 120.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void ATutorialExitPortal::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATutorialExitPortal::OnTriggerBeginOverlap);

	if (!TutorialManager)
	{
		TutorialManager = Cast<ATutorialManager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ATutorialManager::StaticClass())
		);
	}

	if (!TutorialManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL PORTAL] No TutorialManager found in level"));
	}
}

void ATutorialExitPortal::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!TutorialManager || !OtherActor)
	{
		return;
	}

	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (!OverlappingPawn)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[TUTORIAL PORTAL] Overlap by %s"), *GetNameSafe(OverlappingPawn));

	TutorialManager->CompleteTutorialForPawn(OverlappingPawn);
}