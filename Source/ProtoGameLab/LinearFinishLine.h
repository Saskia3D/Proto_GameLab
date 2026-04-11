#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LinearFinishLine.generated.h"

class UBoxComponent;
class UArrowComponent;

UCLASS()
class PROTOGAMELAB_API ALinearFinishLine : public AActor
{
	GENERATED_BODY()

public:
	ALinearFinishLine();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerBox;

	UPROPERTY(VisibleAnywhere)
	UArrowComponent* ArrowComponent;

	// Anti double trigger
	UPROPERTY()
	TSet<TObjectPtr<AActor>> AlreadyTriggered;

	// Paramètres
	UPROPERTY(EditAnywhere, Category = "Finish")
	float MinSpeed = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Finish")
	float MinForwardDot = 0.2f;

	// Overlap
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};