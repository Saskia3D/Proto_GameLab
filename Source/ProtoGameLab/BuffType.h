#pragma once

#include "CoreMinimal.h"
#include "BuffType.generated.h"

UENUM(BlueprintType)
enum class E_BuffType : uint8
{
    //None    UMETA(DisplayName = "None"),
    Turbo   UMETA(DisplayName = "Turbo"),
    TimeStop UMETA(DisplayName = "Time Stop"),
    Projectile  UMETA(DisplayName = "Projectile"),
};