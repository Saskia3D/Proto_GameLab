#pragma once

#include "CoreMinimal.h"
#include "BuffBase.h"
#include "VHSBuff.generated.h"

class UMaterialInterface;

UCLASS()
class PROTOGAMELAB_API UVHSBuff : public UBuffBase
{
	GENERATED_BODY()

public:
	virtual void Activate(APawn* Player) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Audio")
	void OnVHSActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Audio")
	void OnVHSExpired();

protected:
	virtual void OnBuffExpired() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "VHS")
	UMaterialInterface* VHSMaterial;

	TArray<TWeakObjectPtr<class ASTR_RacerPawn>> AffectedPlayers;

	void ApplyVHS();
	void RemoveVHS();
};