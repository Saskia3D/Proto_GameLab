// FinishLine.cpp - Implémentation de la classe AFinishLine, qui représente la ligne d'arrivée dans le jeu

#include "FinishLine.h"
#include "STR_RacerPawn.h"
#include "TimerManager.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
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
	LapByController.Reset();
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

	//on log la velocity avant tout
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
	UE_LOG(LogTemp, Warning, TEXT("[FINISH] Dot=%.3f (MinForwardDot=%.3f)"), Dot, MinForwardDot);

	if (Dot < MinForwardDot)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinishLine ignored: wrong direction (Dot=%.3f)"), Dot);
		return;
	}

	if (AlreadyTriggered.Contains(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: already triggered"));
		return;
	}

	AController* Controller = Pawn->GetController();
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] No controller for pawn %s"), *GetNameSafe(Pawn));
		return;
	}

	ARaceGameMode* GameMode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	UE_LOG(LogTemp, Warning, TEXT("[FINISH] GameMode=%s"), *GetNameSafe(GameMode));

	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: no GameMode"));
		return;
	}

	if (GameMode->IsControllerFinished(Controller))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: controller already finished"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[FINISH] SUCCESS -> Lap logic (%s)"), *GetNameSafe(OtherActor));
	AlreadyTriggered.Add(OtherActor);

	FTimerHandle Tmp;
	GetWorldTimerManager().SetTimer(Tmp, [this, OtherActor]()
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] Timer callback -> NotifyPlayerFinished(%s)"), *GetNameSafe(OtherActor));
		AlreadyTriggered.Remove(OtherActor);
		}, 0.5f, false); // délai de 0.5s pour éviter les problèmes d'overlap multiple

	// Lap counter

	FLapData& Data = LapByController.FindOrAdd(Controller);

	/*if (!Data.bArmed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: not armed (need ArmGate)"));
		return;
	}*/

	// Si course linéaire déjà terminée pour ce joueur -> ignore

	if (TotalLaps == 1 && Data.LapNumber >= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] Linear race already completed"));
		return;
	}

	if (TotalLaps == 1)
	{
		GameMode->NotifyPlayerFinished(Pawn);
		if (!GameMode->bFinishCountdownStarted && GameMode->bUseFinishCountdown)
		{
			GameMode->bFinishCountdownStarted = true;

			UE_LOG(LogTemp, Warning, TEXT("[RACE END] Finish countdown started: %.2fs"), GameMode->FinishCountdownSeconds);

			GetWorldTimerManager().SetTimer(
				GameMode->FinishCountdownHandle,
				GameMode,
				&ARaceGameMode::EndRace,
				GameMode->FinishCountdownSeconds,
				false
			);

			if (GEngine)
			{
				const FString CountdownMsg = FString::Printf(
					TEXT("Final countdown started! Race ends in %.0f seconds."),
					GameMode->FinishCountdownSeconds
				);
				GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, CountdownMsg);
			}
		}
		GameMode->EndRace();
		return;
	}

	if (TotalLaps > 1 && !Data.bArmed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: not armed (need ArmGate)"));
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - Data.LastCrossTime < LapCooldownSeconds)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] RETURN: lap cooldown"));
		return;
	}

	Data.LastCrossTime = Now;
	Data.LapNumber++;
	
	if (TotalLaps > 1)
	{
		Data.bArmed = false;
	}

	// A chaque lap, on appelle NotifyLapCompleted
	if (GameMode)
	{
		GameMode->NotifyLapCompleted(Controller, Data.LapNumber);
	}

	UE_LOG(LogTemp, Warning, TEXT("[FINISH] LAP++ Controller=%s Lap=%d/%d"),
		*GetNameSafe(Controller), Data.LapNumber, TotalLaps);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			FString::Printf(TEXT("Lap %d/%d"), Data.LapNumber, TotalLaps));
	}

	if (Data.LapNumber >= TotalLaps)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FINISH] Player reached MaxLaps -> NotifyPlayerFinished(%s)"),
			*GetNameSafe(Pawn));

		GameMode->NotifyPlayerFinished(Pawn);
	}
}

// Called every frame
void AFinishLine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFinishLine::ArmForController(AController* Controller)
{
	if (!Controller) return;
	FLapData& Data = LapByController.FindOrAdd(Controller);
	Data.bArmed = true;
}

