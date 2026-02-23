#pragma once

#include "CoreMinimal.h"
#include "OmniDebugTypes.generated.h"

UENUM(BlueprintType)
enum class EOmniDebugLevel : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Error UMETA(DisplayName = "Error"),
	Event UMETA(DisplayName = "Event")
};

UENUM(BlueprintType)
enum class EOmniDebugMode : uint8
{
	Off UMETA(DisplayName = "Off"),
	Basic UMETA(DisplayName = "Basic"),
	Full UMETA(DisplayName = "Full")
};

USTRUCT(BlueprintType)
struct FOmniDebugEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	FString TimestampUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	float WorldTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	EOmniDebugLevel Level = EOmniDebugLevel::Info;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	FName Category = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	FName Source = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	FString Message;
};

USTRUCT(BlueprintType)
struct FOmniDebugMetric
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	FName Key = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Debug")
	FString Value;
};
