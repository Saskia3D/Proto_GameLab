// Checkpoint.cpp

#include "Checkpoint.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "RaceGameMode.h"

ACheckpoint::ACheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger")); // Créer un composant de collision de type BoxComponent pour le trigger
	SetRootComponent(Trigger); // Définir le composant de collision comme racine de l'acteur

	// Configurer les paramètres de collision du trigger pour détecter les overlaps
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
}

void ACheckpoint::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::OnBeginOverlap); // Lier la fonction OnBeginOverlap à l'événement de début de chevauchement du composant Trigger
}

void ACheckpoint::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	UE_LOG(LogTemp, Warning, TEXT("Checkpoint overlap: OtherActor=%s"), *GetNameSafe(OtherActor));

	// Vérifier si l'acteur qui chevauche est un Pawn (un véhicule dans ce cas)
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	// Notifiez le GameMode que ce Pawn a passé ce checkpoint
	ARaceGameMode* GameMode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GameMode) return;

	GameMode->NotifyCheckpointPassed(Pawn, CheckpointIndex); // Appeler une fonction dans le GameMode pour gérer la logique de passage du checkpoint, en passant le Pawn et l'index du checkpoint
}

