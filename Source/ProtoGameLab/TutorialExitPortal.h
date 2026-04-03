#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialExitPortal.generated.h"

class UBoxComponent;
class ATutorialManager;

UCLASS()
class PROTOGAMELAB_API ATutorialExitPortal : public AActor
{
	GENERATED_BODY()

public:
	ATutorialExitPortal();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TObjectPtr<UBoxComponent> TriggerBox = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial")
	TObjectPtr<ATutorialManager> TutorialManager = nullptr;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
};
