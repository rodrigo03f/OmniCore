#pragma once

#include "CoreMinimal.h"
#include "OmniSystemMessaging.generated.h"

USTRUCT(BlueprintType)
struct OMNICORE_API FOmniCommandMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName SourceSystem = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName TargetSystem = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName CommandName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	TMap<FName, FString> Arguments;
};

USTRUCT(BlueprintType)
struct OMNICORE_API FOmniQueryMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName SourceSystem = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName TargetSystem = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName QueryName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	TMap<FName, FString> Arguments;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Messaging")
	bool bHandled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Messaging")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Messaging")
	FString Result;

	UPROPERTY(BlueprintReadOnly, Category = "Omni|Messaging")
	TMap<FName, FString> Output;
};

USTRUCT(BlueprintType)
struct OMNICORE_API FOmniEventMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName SourceSystem = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	FName EventName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Omni|Messaging")
	TMap<FName, FString> Payload;
};
