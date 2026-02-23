#pragma once

#include "CoreMinimal.h"
#include "Systems/OmniSystemMessaging.h"

namespace OmniMessageSchema
{
	OMNICORE_API extern const FName SystemMovement;
	OMNICORE_API extern const FName SystemActionGate;
	OMNICORE_API extern const FName SystemStatus;

	OMNICORE_API extern const FName CommandStartAction;
	OMNICORE_API extern const FName CommandStopAction;
	OMNICORE_API extern const FName CommandSetSprinting;
	OMNICORE_API extern const FName QueryCanStartAction;
	OMNICORE_API extern const FName QueryIsExhausted;
	OMNICORE_API extern const FName QueryGetStateTagsCsv;
	OMNICORE_API extern const FName EventExhausted;
	OMNICORE_API extern const FName EventExhaustedCleared;
}

struct OMNICORE_API FOmniStartActionCommandSchema
{
	FName SourceSystem = NAME_None;
	FName ActionId = NAME_None;

	static FOmniCommandMessage ToMessage(const FOmniStartActionCommandSchema& Data);
	static bool TryFromMessage(const FOmniCommandMessage& Message, FOmniStartActionCommandSchema& OutData, FString& OutError);
	static bool Validate(const FOmniCommandMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniStopActionCommandSchema
{
	FName SourceSystem = NAME_None;
	FName ActionId = NAME_None;
	FName Reason = NAME_None;

	static FOmniCommandMessage ToMessage(const FOmniStopActionCommandSchema& Data);
	static bool TryFromMessage(const FOmniCommandMessage& Message, FOmniStopActionCommandSchema& OutData, FString& OutError);
	static bool Validate(const FOmniCommandMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniSetSprintingCommandSchema
{
	FName SourceSystem = NAME_None;
	bool bSprinting = false;

	static FOmniCommandMessage ToMessage(const FOmniSetSprintingCommandSchema& Data);
	static bool TryFromMessage(const FOmniCommandMessage& Message, FOmniSetSprintingCommandSchema& OutData, FString& OutError);
	static bool Validate(const FOmniCommandMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniCanStartActionQuerySchema
{
	FName SourceSystem = NAME_None;
	FName ActionId = NAME_None;
	bool bAllowed = false;
	FString Reason;

	static FOmniQueryMessage ToMessage(const FOmniCanStartActionQuerySchema& Data);
	static bool TryFromMessage(const FOmniQueryMessage& Message, FOmniCanStartActionQuerySchema& OutData, FString& OutError);
	static bool Validate(const FOmniQueryMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniIsExhaustedQuerySchema
{
	FName SourceSystem = NAME_None;
	bool bExhausted = false;

	static FOmniQueryMessage ToMessage(const FOmniIsExhaustedQuerySchema& Data);
	static bool TryFromMessage(const FOmniQueryMessage& Message, FOmniIsExhaustedQuerySchema& OutData, FString& OutError);
	static bool Validate(const FOmniQueryMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniGetStateTagsCsvQuerySchema
{
	FName SourceSystem = NAME_None;
	FString TagsCsv;

	static FOmniQueryMessage ToMessage(const FOmniGetStateTagsCsvQuerySchema& Data);
	static bool TryFromMessage(const FOmniQueryMessage& Message, FOmniGetStateTagsCsvQuerySchema& OutData, FString& OutError);
	static bool Validate(const FOmniQueryMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniExhaustedEventSchema
{
	FName SourceSystem = NAME_None;
	bool bExhausted = true;

	static FOmniEventMessage ToMessage(const FOmniExhaustedEventSchema& Data);
	static bool TryFromMessage(const FOmniEventMessage& Message, FOmniExhaustedEventSchema& OutData, FString& OutError);
	static bool Validate(const FOmniEventMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniExhaustedClearedEventSchema
{
	FName SourceSystem = NAME_None;
	bool bExhausted = false;

	static FOmniEventMessage ToMessage(const FOmniExhaustedClearedEventSchema& Data);
	static bool TryFromMessage(const FOmniEventMessage& Message, FOmniExhaustedClearedEventSchema& OutData, FString& OutError);
	static bool Validate(const FOmniEventMessage& Message, FString& OutError);
};

struct OMNICORE_API FOmniMessageSchemaValidator
{
	static bool ValidateCommand(const FOmniCommandMessage& Message, FString& OutError);
	static bool ValidateQuery(const FOmniQueryMessage& Message, FString& OutError);
	static bool ValidateEvent(const FOmniEventMessage& Message, FString& OutError);
};
