// TimeStopBuff.h

#pragma once

#include "CoreMinimal.h"
#include "BuffBase.h"
#include "TimeStopBuff.generated.h"

class AActor;

/**
 * 
 */
UCLASS()
class PROTOGAMELAB_API UTimeStopBuff : public UBuffBase
{
	GENERATED_BODY()
	
public:
	virtual void Activate(APawn* Player) override;

protected:
	virtual void OnBuffExpired() override;

	//Tag des objets a freeze (obstacles, hazards, joueurs...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TimeStop")
	FName AffectableTag = TEXT("TimeStopAffectable");

private:
	TMap<TWeakObjectPtr<AActor>, float> SavedDilations; //Sauvegarde pour restore

	void ApplyTimeStop();
	void RestoreTimeStop();
};
