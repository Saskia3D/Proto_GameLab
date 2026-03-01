// LapArmGate.cpp

#include "LapArmGate.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "FinishLine.h"

// Sets default values
ALapArmGate::ALapArmGate()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);

	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
}

// Called when the game starts or when spawned
void ALapArmGate::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ALapArmGate::OnOverlapBegin);
}

void ALapArmGate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this) return;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	AController* Controller = Pawn->GetController();
	if (!Controller) return;

	TArray<AActor*> FinishLines;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFinishLine::StaticClass(), FinishLines);

	if (FinishLines.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ARMGATE] No FinishLine found in level"));
		return;
	}

	AFinishLine* FinishLine = Cast<AFinishLine>(FinishLines[0]);
	if (!FinishLine) return;

	FinishLine->ArmForController(Controller);

	UE_LOG(LogTemp, Warning, TEXT("[ARMGATE] Armed controller %s"),
		*GetNameSafe(Controller));
}