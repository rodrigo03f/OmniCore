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

	void ResetArguments()
	{
		Arguments.Reset();
	}

	void SetArgument(const FName Key, const FString& Value)
	{
		Arguments.Add(Key, Value);
	}

	bool TryGetArgument(const FName Key, FString& OutValue) const
	{
		if (const FString* Value = Arguments.Find(Key))
		{
			OutValue = *Value;
			return true;
		}

		return false;
	}
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

	void ResetArguments()
	{
		Arguments.Reset();
	}

	void SetArgument(const FName Key, const FString& Value)
	{
		Arguments.Add(Key, Value);
	}

	bool TryGetArgument(const FName Key, FString& OutValue) const
	{
		if (const FString* Value = Arguments.Find(Key))
		{
			OutValue = *Value;
			return true;
		}

		return false;
	}

	void ResetOutput()
	{
		Output.Reset();
	}

	void SetOutputValue(const FName Key, const FString& Value)
	{
		Output.Add(Key, Value);
	}

	bool TryGetOutputValue(const FName Key, FString& OutValue) const
	{
		if (const FString* Value = Output.Find(Key))
		{
			OutValue = *Value;
			return true;
		}

		return false;
	}

	void ResetResponse()
	{
		bHandled = false;
		bSuccess = false;
		Result.Reset();
		Output.Reset();
	}
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

	void ResetPayload()
	{
		Payload.Reset();
	}

	void SetPayloadValue(const FName Key, const FString& Value)
	{
		Payload.Add(Key, Value);
	}

	bool TryGetPayloadValue(const FName Key, FString& OutValue) const
	{
		if (const FString* Value = Payload.Find(Key))
		{
			OutValue = *Value;
			return true;
		}

		return false;
	}
};
