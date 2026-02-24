// FinishLine.cpp - Implémentation de la classe AFinishLine, qui représente la ligne d'arrivée dans le jeu


#include "FinishLine.h"
#include "STR_RacerPawn.h"
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

	// Créer le composant de flèche pour indiquer la direction de la ligne d'arrivée
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
	ArrowComponent->SetRelativeLocation(FVector::ZeroVector);
	ArrowComponent->SetRelativeRotation(FRotator::ZeroRotator);
	ArrowComponent->ArrowSize = 2.0f;
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

void AFinishLine::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[FINISH] Overlap with: %s | OtherComp=%s"),
		*GetNameSafe(OtherActor), *GetNameSafe(OtherComp));

	if (!OtherActor || OtherActor == this || !OtherComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: invalid actor/comp"));
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: not a Pawn"));
		return;
	}

	AController* C = Pawn->GetController();
	UE_LOG(LogTemp, Warning, TEXT("[FINISH] Pawn=%s Controller=%s IsPlayerControlled=%d"),
		*GetNameSafe(Pawn), *GetNameSafe(C), Pawn->IsPlayerControlled());

	// IMPORTANT: on log la velocity avant tout
	const FVector V = Pawn->GetVelocity();
	float Speed = Pawn->GetVelocity().Size();
	FVector MoveDir = Pawn->GetVelocity().GetSafeNormal();

	if (ASTR_RacerPawn* STR = Cast<ASTR_RacerPawn>(Pawn))
	{
		Speed = STR->GetCurrentSpeed();          // vitesse interne (fiable)
		MoveDir = STR->GetActorForwardVector();  // il avance dans son forward
	}

	if (Speed < MinSpeed)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinishLine ignored: too slow (Speed=%.2f)"), Speed);
		return;
	}

	const FVector ForwardVector = ArrowComponent ? ArrowComponent->GetForwardVector() : GetActorForwardVector();
	const float Dot = FVector::DotProduct(MoveDir, ForwardVector);
	//const FVector ForwardVector = ArrowComponent ? ArrowComponent->GetForwardVector() : GetActorForwardVector();
	//const float Dot = FVector::DotProduct(V.GetSafeNormal(), ForwardVector);
	UE_LOG(LogTemp, Warning, TEXT("[FINISH] Dot=%.3f (MinForwardDot=%.3f)"), Dot, MinForwardDot);

	if (Dot < MinForwardDot)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinishLine ignored: wrong direction (Dot=%.3f)"), Dot);
		return;
	}

	/*if (Dot < MinForwardDot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: wrong direction"));
		return;
	}*/

	if (AlreadyTriggered.Contains(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: already triggered"));
		return;
	}

	ARaceGameMode* GameMode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	UE_LOG(LogTemp, Warning, TEXT("[FINISH] GameMode=%s"), *GetNameSafe(GameMode));

	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: no GameMode"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[FINISH] SUCCESS -> NotifyPlayerFinished(%s)"), *GetNameSafe(OtherActor));
	AlreadyTriggered.Add(OtherActor);
	GameMode->NotifyPlayerFinished(OtherActor);
}

/*
void AFinishLine::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("FinishLine Overlap with: %s"), *GetNameSafe(OtherActor)); // Log pour vérifier que la fonction d'overlap est appelée et quel acteur a déclenché l'overlap

	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		if (!IsPlayerVehicle(OtherActor))
		{
			return; // Ignore si ce n'est pas un véhicule du joueur
		}
		
		APawn* Pawn = Cast<APawn>(OtherActor);
		if(!Pawn)
		{
			return; // Ignore si ce n'est pas un Pawn
		}

		const FVector Velocity = Pawn->GetVelocity(); // Obtenir la vitesse du véhicule pour vérifier si trop lent
		const float Speed = Velocity.Size(); // Calculer la vitesse en unités Unreal (cm/s)

		if(Speed < MinSpeed) // Seuil de vitesse
		{
			return; // Ignore si le véhicule est trop lent
		}

		const FVector ForwardVector = ArrowComponent ? ArrowComponent->GetForwardVector() : GetActorForwardVector(); // Obtenir la direction de la ligne d'arrivée

		const float Dot = FVector::DotProduct(Velocity.GetSafeNormal(), ForwardVector); // Calculer le dot product pour vérifier l'orientation du véhicule

		UE_LOG(LogTemp, Warning, TEXT("Speed=%.2f Dot=%.2f"), Speed, Dot); // Log pour vérifier les valeurs de vitesse et d'orientation du véhicule lors de l'overlap

		if(Dot < MinForwardDot) // Seuil d'orientation
		{
			UE_LOG(LogTemp, Warning, TEXT("FinishLine Overlap ignored, wrong direction: %s"), *GetNameSafe(OtherActor)); // Log pour vérifier que l'overlap est ignoré à cause de l'orientation
			return; // Ignore si le véhicule n'est pas orienté dans la bonne direction
		}

		ARaceGameMode* GameMode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(GetWorld())); // Obtenir le GameMode pour notifier que le joueur a terminé la course

		if(GameMode)
		{
			if(AlreadyTriggered.Contains(OtherActor))
			{
				return; // Ignore si ce joueur a déjà déclenché la ligne d'arrivée
			}
			AlreadyTriggered.Add(OtherActor); // Ajouter le joueur à l'ensemble des joueurs qui ont déjà déclenché la ligne d'arrivée
			GameMode->NotifyPlayerFinished(OtherActor); // Notifier le GameMode que le joueur a terminé la course
			return;
		}
	}
}*/

// Called every frame
void AFinishLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

