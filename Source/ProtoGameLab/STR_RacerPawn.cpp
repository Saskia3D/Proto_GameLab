// STR_RacerPawn.cpp - Implémentation de la classe ASTR_RacerPawn, qui représente le véhicule contrôlé par le joueur dans le jeu

#include "STR_RacerPawn.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "PaperSpriteComponent.h"
#include "EnhancedInputComponent.h"
#include "BuffComponent.h"
#include "EnhancedInputSubsystems.h"

ASTR_RacerPawn::ASTR_RacerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Setup de la Capsule (Collision)
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->SetCapsuleSize(40.f, 40.f);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComp->SetGenerateOverlapEvents(true);
	CapsuleComp->SetCollisionObjectType(ECC_Pawn);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CapsuleComp->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CapsuleComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	//CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//CapsuleComp->SetGenerateOverlapEvents(true);
	//CapsuleComp->SetCollisionResponseToAllChannels(ECR_Overlap);

	// 2. Setup du Sprite
	SpriteComp = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComp"));
	SpriteComp->SetupAttachment(RootComponent);
	SpriteComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 90.0f)); // À plat
	SpriteComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteComp->SetGenerateOverlapEvents(false);

	// 3. Setup de la Caméra
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->SetUsingAbsoluteRotation(true);
	SpringArmComp->TargetArmLength = 800.0f;
	SpringArmComp->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	//FoundBuffComp = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComp"));
	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));

	// Valeurs par défaut
	CurrentSpeed = 0.0f;
	bIsBraking = false;
}

void ASTR_RacerPawn::BeginPlay()
{
	Super::BeginPlay();

	/*if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Sécurité : On vérifie que le mapping context existe avant de l'ajouter
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}*/

}

void ASTR_RacerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- 1. GESTION VITESSE ---
	if (bIsBraking)
	{
		CurrentSpeed -= BrakingDeceleration * DeltaTime;
	}
	else
	{
		CurrentSpeed += AccelerationRate * DeltaTime;
	}

	CurrentSpeed = FMath::Clamp(CurrentSpeed, 0.0f, MaxSpeed);

	// --- 2. GESTION ROTATION ---
	if (!MovementInput.IsZero())
	{
		FVector Direction = FVector(MovementInput.X, MovementInput.Y, 0.0f);
		if (!Direction.IsNearlyZero())
		{
			SetActorRotation(Direction.Rotation());
		}
	}

	// --- 3. MOUVEMENT ---
	const FVector Delta = GetActorForwardVector() * CurrentSpeed * DeltaTime;

	FHitResult Hit;
	CapsuleComp->MoveComponent(Delta, GetActorRotation(), true, &Hit);

	if (DeltaTime > 0.f)
	{
		CapsuleComp->ComponentVelocity = Delta / DeltaTime;
	}

	// --- 4. DEBUG OVERLAPS ---
	TArray<AActor*> Overlapping;
	CapsuleComp->GetOverlappingActors(Overlapping);

	int32 CountCP = 0;
	for (AActor* A : Overlapping)
	{
		if (A && A->GetName().Contains(TEXT("Checkpoint")))
		{
			CountCP++;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Overlaps=%d  CheckpointOverlaps=%d"), Overlapping.Num(), CountCP);
}

void ASTR_RacerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Ajout de "if (MoveAction)" pour éviter un crash si l'action n'est pas assignée dans le Blueprint
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASTR_RacerPawn::Move);
			// Astuce : Quand on lâche le stick, on arrête de tourner (Optionnel mais mieux)
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASTR_RacerPawn::Move);
		}

		if (BrakeAction)
		{
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ASTR_RacerPawn::StartBrake);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ASTR_RacerPawn::StopBrake);
		}

		if (ItemAction)
		{
			EnhancedInputComponent->BindAction(ItemAction, ETriggerEvent::Started, this, &ASTR_RacerPawn::UseItem);
		}
	}
}

void ASTR_RacerPawn::Move(const FInputActionValue& Value)
{
	MovementInput = Value.Get<FVector2D>();
}

void ASTR_RacerPawn::StartBrake(const FInputActionValue& Value)
{
	bIsBraking = true;
}

void ASTR_RacerPawn::StopBrake(const FInputActionValue& Value)
{
	bIsBraking = false;
}

void ASTR_RacerPawn::UseItem(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[ITEM] UseItem called on %s"), *GetName());

	/*UBuffComponent* BuffComp = FindComponentByClass<UBuffComponent>();

	if (BuffComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ITEM] BuffComp found. HasBuff=%d  CurrentBuffPtr=%p"),
			BuffComp->CurrentBuff != nullptr,
			BuffComp->CurrentBuff);

		BuffComp->UseBuff();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ITEM] NO BuffComponent on this pawn!"));
	}*/

	if (BuffComponent)
	{
		BuffComponent->UseBuff();
	}
}

void ASTR_RacerPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	APlayerController* PC = Cast<APlayerController>(NewController);
	if (!PC) return;

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);

	if (Subsystem && DefaultMappingContext)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void ASTR_RacerPawn::UnPossessed()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				if (DefaultMappingContext)
				{
					Subsystem->RemoveMappingContext(DefaultMappingContext);
				}
			}
		}
	}

	Super::UnPossessed();
}