// Fill out your copyright notice in the Description page of Project Settings.

#include "MysteryBox.h"
#include "Components/BoxComponent.h"
#include "BuffBase.h"
#include "MyVehiclePawn.h"
#include "STR_RacerPawn.h"
#include "BuffComponent.h"

// Sets default values
AMysteryBox::AMysteryBox()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;

    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionBox->SetGenerateOverlapEvents(true);


    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AMysteryBox::OnOverlapBegin);
}

// Called when the game starts or when spawned
void AMysteryBox::BeginPlay()
{
    Super::BeginPlay();

}

void AMysteryBox::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    /*
    AMyVehiclePawn* Vehicle = Cast<AMyVehiclePawn>(OtherActor);

    if (Vehicle && PossibleBuffs.Num() > 0)
    {
        int32 Index = FMath::RandRange(0, PossibleBuffs.Num() - 1);
        TSubclassOf<UBuffBase> SelectedBuff = PossibleBuffs[Index];

        UBuffBase* NewBuff = NewObject<UBuffBase>(Vehicle, SelectedBuff);

        Vehicle->SetStoredBuff(NewBuff);

        UE_LOG(LogTemp, Warning, TEXT("MysteryBox: Buff stocké !"));

        Destroy();
    }*/

    ASTR_RacerPawn* RacerPawn = Cast<ASTR_RacerPawn>(OtherActor);

    if (RacerPawn && PossibleBuffs.Num() > 0)
    {
        int32 Index = FMath::RandRange(0, PossibleBuffs.Num() - 1);
        TSubclassOf<UBuffBase> SelectedBuff = PossibleBuffs[Index];

        if (UBuffComponent* FoundBuffComp = RacerPawn->FindComponentByClass<UBuffComponent>())
        {
            FoundBuffComp->AddBuff(SelectedBuff);
            UE_LOG(LogTemp, Warning, TEXT("MysteryBox: Buff added to BuffComponent!"));
            Destroy();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("MysteryBox: Racer has NO BuffComponent!"));
        }
    }
}
