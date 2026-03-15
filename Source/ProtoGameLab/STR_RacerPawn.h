// STR_RacerPawn.h

#pragma once

#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h" // Nécessite le module EnhancedInput
#include "STR_RacerPawn.generated.h"

// Forward declarations (Pour éviter les erreurs d'inclusions circulaires)
class UCapsuleComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UPaperSpriteComponent;
class UBuffComponent;

UCLASS()
class PROTOGAMELAB_API ASTR_RacerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTR_RacerPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override; // Surcharge de la fonction PossessedBy pour ajouter des fonctionnalités lors de la possession du Pawn
	virtual void UnPossessed() override; // Surcharge de la fonction UnPossessed pour ajouter des fonctionnalités lors de la dépossession du Pawn

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- COMPOSANTS ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperSpriteComponent* SpriteComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	// --- INPUTS ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SteerAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* BrakeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ItemAction;

	// --- PARAMÈTRES ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AccelerationRate = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BrakingDeceleration = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement|Steering")
	float MaxTurnRate = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement|Steering")
	float MinTurnRateAtMaxSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement|Steering")
	float SteeringInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement|Steering")
	float MinSpeedToTurn = 40.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Buff")
	//UBuffComponent* FoundBuffComp = FindComponentByClass<UBuffComponent>();
	UBuffComponent* BuffComponent;

	// --- ASSETS ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
	TArray<UPaperSprite*> CarSprites;

	UFUNCTION(BlueprintCallable, Category="Mouvement")
	float GetCurrentSpeed() const { return CurrentSpeed; } // Getter pour la vitesse actuelle, utile pour les Blueprints

protected:
	float CurrentSpeed = 0.f;
	float TargetSteeringInput = 0.f;
	float CurrentSteeringInput = 0.f;
	//FVector2D MovementInput;
	bool bIsBraking;

	// Fonctions Inputs
	void Steer(const FInputActionValue& Value);
	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);
	void UseItem(const FInputActionValue& Value);
};