// FinishLine.cpp - Implémentation de la classe AFinishLine, qui représente la ligne d'arrivée dans le jeu


#include "FinishLine.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RaceGameMode.h"

// Sets default values
AFinishLine::AFinishLine()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Créer le composant de collision
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);

	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f)); // Définir la taille du box

	// Configurer les paramètres de collision pour que le box puisse détecter les overlaps avec les véhicules du joueur
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);

}

// Called when the game starts or when spawned
void AFinishLine::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AFinishLine::OnOverlapBegin); // Lier la fonction d'overlap
	UE_LOG(LogTemp, Warning, TEXT("FinishLine BeginPlay: %s"), *GetName()); // Log pour vérifier que le BeginPlay est appelé
	UE_LOG(LogTemp, Warning, TEXT("RaceGameMode BeginPlay")); // Log pour vérifier que le BeginPlay du GameMode est appelé
}

bool AFinishLine::IsPlayerVehicle(AActor* Actor) const
{
	APawn* Pawn = Cast<APawn>(Actor);
	if (Pawn)
	{
		APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
		return PlayerController != nullptr; // Vérifie si le contrôleur est un PlayerController
	}
	return false;
}

void AFinishLine::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("FinishLine Overlap with: %s"), *GetNameSafe(OtherActor)); // Log pour vérifier que la fonction d'overlap est appelée et quel acteur a déclenché l'overlap

	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		if (!IsPlayerVehicle(OtherActor))
		{
			return; // Ignore si ce n'est pas un véhicule du joueur
		}
		
		ARaceGameMode* GameMode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(GetWorld())); // Obtenir le GameMode pour notifier que le joueur a terminé la course

		if(GameMode)
		{
			GameMode->NotifyPlayerFinished(OtherActor); // Notifier le GameMode que le joueur a terminé la course
			return;
		}
	}
}

// Called every frame
void AFinishLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

