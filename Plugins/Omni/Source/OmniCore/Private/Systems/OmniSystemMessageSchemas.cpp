#include "Systems/OmniSystemMessageSchemas.h"

namespace OmniMessageSchema
{
	const FName SystemMovement(TEXT("Movement"));
	const FName SystemActionGate(TEXT("ActionGate"));
	const FName SystemStatus(TEXT("Status"));

	const FName CommandStartAction(TEXT("StartAction"));
	const FName CommandStopAction(TEXT("StopAction"));
	const FName CommandSetSprinting(TEXT("SetSprinting"));
	const FName QueryCanStartAction(TEXT("CanStartAction"));
	const FName QueryIsExhausted(TEXT("IsExhausted"));
	const FName QueryGetStateTagsCsv(TEXT("GetStateTagsCsv"));
	const FName EventExhausted(TEXT("Exhausted"));
	const FName EventExhaustedCleared(TEXT("ExhaustedCleared"));
}

namespace
{
	static bool TryGetRequiredValue(const TMap<FName, FString>& Map, const FName Key, FString& OutValue, FString& OutError)
	{
		const FString* FoundValue = Map.Find(Key);
		if (!FoundValue)
		{
			OutError = FString::Printf(TEXT("Missing required key '%s'."), *Key.ToString());
			return false;
		}

		OutValue = *FoundValue;
		return true;
	}

	static bool TryParseBool(const FString& Value, bool& OutBool)
	{
		if (Value.Equals(TEXT("True"), ESearchCase::IgnoreCase) || Value == TEXT("1"))
		{
			OutBool = true;
			return true;
		}
		if (Value.Equals(TEXT("False"), ESearchCase::IgnoreCase) || Value == TEXT("0"))
		{
			OutBool = false;
			return true;
		}
		return false;
	}
}

FOmniCommandMessage FOmniStartActionCommandSchema::ToMessage(const FOmniStartActionCommandSchema& Data)
{
	FOmniCommandMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.TargetSystem = OmniMessageSchema::SystemActionGate;
	Message.CommandName = OmniMessageSchema::CommandStartAction;
	Message.Arguments.Add(TEXT("ActionId"), Data.ActionId.ToString());
	return Message;
}

bool FOmniStartActionCommandSchema::TryFromMessage(
	const FOmniCommandMessage& Message,
	FOmniStartActionCommandSchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.ActionId = FName(*Message.Arguments.FindChecked(TEXT("ActionId")));
	return true;
}

bool FOmniStartActionCommandSchema::Validate(const FOmniCommandMessage& Message, FString& OutError)
{
	if (Message.TargetSystem != OmniMessageSchema::SystemActionGate || Message.CommandName != OmniMessageSchema::CommandStartAction)
	{
		OutError = TEXT("Command schema mismatch for StartAction.");
		return false;
	}

	FString ActionIdValue;
	if (!TryGetRequiredValue(Message.Arguments, TEXT("ActionId"), ActionIdValue, OutError))
	{
		return false;
	}
	if (ActionIdValue.IsEmpty() || FName(*ActionIdValue) == NAME_None)
	{
		OutError = TEXT("Invalid ActionId.");
		return false;
	}

	return true;
}

FOmniCommandMessage FOmniStopActionCommandSchema::ToMessage(const FOmniStopActionCommandSchema& Data)
{
	FOmniCommandMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.TargetSystem = OmniMessageSchema::SystemActionGate;
	Message.CommandName = OmniMessageSchema::CommandStopAction;
	Message.Arguments.Add(TEXT("ActionId"), Data.ActionId.ToString());
	if (Data.Reason != NAME_None)
	{
		Message.Arguments.Add(TEXT("Reason"), Data.Reason.ToString());
	}
	return Message;
}

bool FOmniStopActionCommandSchema::TryFromMessage(
	const FOmniCommandMessage& Message,
	FOmniStopActionCommandSchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.ActionId = FName(*Message.Arguments.FindChecked(TEXT("ActionId")));
	if (const FString* ReasonValue = Message.Arguments.Find(TEXT("Reason")))
	{
		OutData.Reason = FName(**ReasonValue);
	}
	return true;
}

bool FOmniStopActionCommandSchema::Validate(const FOmniCommandMessage& Message, FString& OutError)
{
	if (Message.TargetSystem != OmniMessageSchema::SystemActionGate || Message.CommandName != OmniMessageSchema::CommandStopAction)
	{
		OutError = TEXT("Command schema mismatch for StopAction.");
		return false;
	}

	FString ActionIdValue;
	if (!TryGetRequiredValue(Message.Arguments, TEXT("ActionId"), ActionIdValue, OutError))
	{
		return false;
	}
	if (ActionIdValue.IsEmpty() || FName(*ActionIdValue) == NAME_None)
	{
		OutError = TEXT("Invalid ActionId.");
		return false;
	}

	return true;
}

FOmniCommandMessage FOmniSetSprintingCommandSchema::ToMessage(const FOmniSetSprintingCommandSchema& Data)
{
	FOmniCommandMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.TargetSystem = OmniMessageSchema::SystemStatus;
	Message.CommandName = OmniMessageSchema::CommandSetSprinting;
	Message.Arguments.Add(TEXT("bSprinting"), Data.bSprinting ? TEXT("True") : TEXT("False"));
	return Message;
}

bool FOmniSetSprintingCommandSchema::TryFromMessage(
	const FOmniCommandMessage& Message,
	FOmniSetSprintingCommandSchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	FString SprintingValue;
	TryGetRequiredValue(Message.Arguments, TEXT("bSprinting"), SprintingValue, OutError);

	bool bSprinting = false;
	if (!TryParseBool(SprintingValue, bSprinting))
	{
		OutError = TEXT("Invalid boolean value for bSprinting.");
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.bSprinting = bSprinting;
	return true;
}

bool FOmniSetSprintingCommandSchema::Validate(const FOmniCommandMessage& Message, FString& OutError)
{
	if (Message.TargetSystem != OmniMessageSchema::SystemStatus || Message.CommandName != OmniMessageSchema::CommandSetSprinting)
	{
		OutError = TEXT("Command schema mismatch for SetSprinting.");
		return false;
	}

	FString SprintingValue;
	if (!TryGetRequiredValue(Message.Arguments, TEXT("bSprinting"), SprintingValue, OutError))
	{
		return false;
	}

	bool bParsedValue = false;
	if (!TryParseBool(SprintingValue, bParsedValue))
	{
		OutError = TEXT("Invalid boolean value for bSprinting.");
		return false;
	}

	return true;
}

FOmniQueryMessage FOmniCanStartActionQuerySchema::ToMessage(const FOmniCanStartActionQuerySchema& Data)
{
	FOmniQueryMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.TargetSystem = OmniMessageSchema::SystemActionGate;
	Message.QueryName = OmniMessageSchema::QueryCanStartAction;
	Message.Arguments.Add(TEXT("ActionId"), Data.ActionId.ToString());
	return Message;
}

bool FOmniCanStartActionQuerySchema::TryFromMessage(
	const FOmniQueryMessage& Message,
	FOmniCanStartActionQuerySchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.ActionId = FName(*Message.Arguments.FindChecked(TEXT("ActionId")));
	OutData.bAllowed = Message.bSuccess;
	if (const FString* ReasonValue = Message.Output.Find(TEXT("Reason")))
	{
		OutData.Reason = *ReasonValue;
	}
	else
	{
		OutData.Reason = Message.Result;
	}
	return true;
}

bool FOmniCanStartActionQuerySchema::Validate(const FOmniQueryMessage& Message, FString& OutError)
{
	if (Message.TargetSystem != OmniMessageSchema::SystemActionGate || Message.QueryName != OmniMessageSchema::QueryCanStartAction)
	{
		OutError = TEXT("Query schema mismatch for CanStartAction.");
		return false;
	}

	FString ActionIdValue;
	if (!TryGetRequiredValue(Message.Arguments, TEXT("ActionId"), ActionIdValue, OutError))
	{
		return false;
	}
	if (ActionIdValue.IsEmpty() || FName(*ActionIdValue) == NAME_None)
	{
		OutError = TEXT("Invalid ActionId.");
		return false;
	}

	return true;
}

FOmniQueryMessage FOmniIsExhaustedQuerySchema::ToMessage(const FOmniIsExhaustedQuerySchema& Data)
{
	FOmniQueryMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.TargetSystem = OmniMessageSchema::SystemStatus;
	Message.QueryName = OmniMessageSchema::QueryIsExhausted;
	return Message;
}

bool FOmniIsExhaustedQuerySchema::TryFromMessage(
	const FOmniQueryMessage& Message,
	FOmniIsExhaustedQuerySchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	bool bExhausted = false;
	if (!TryParseBool(Message.Result, bExhausted))
	{
		OutError = TEXT("Invalid query result for IsExhausted.");
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.bExhausted = bExhausted;
	return true;
}

bool FOmniIsExhaustedQuerySchema::Validate(const FOmniQueryMessage& Message, FString& OutError)
{
	if (Message.TargetSystem != OmniMessageSchema::SystemStatus || Message.QueryName != OmniMessageSchema::QueryIsExhausted)
	{
		OutError = TEXT("Query schema mismatch for IsExhausted.");
		return false;
	}

	return true;
}

FOmniQueryMessage FOmniGetStateTagsCsvQuerySchema::ToMessage(const FOmniGetStateTagsCsvQuerySchema& Data)
{
	FOmniQueryMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.TargetSystem = OmniMessageSchema::SystemStatus;
	Message.QueryName = OmniMessageSchema::QueryGetStateTagsCsv;
	return Message;
}

bool FOmniGetStateTagsCsvQuerySchema::TryFromMessage(
	const FOmniQueryMessage& Message,
	FOmniGetStateTagsCsvQuerySchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.TagsCsv = Message.Result;
	return true;
}

bool FOmniGetStateTagsCsvQuerySchema::Validate(const FOmniQueryMessage& Message, FString& OutError)
{
	if (Message.TargetSystem != OmniMessageSchema::SystemStatus || Message.QueryName != OmniMessageSchema::QueryGetStateTagsCsv)
	{
		OutError = TEXT("Query schema mismatch for GetStateTagsCsv.");
		return false;
	}

	return true;
}

FOmniEventMessage FOmniExhaustedEventSchema::ToMessage(const FOmniExhaustedEventSchema& Data)
{
	FOmniEventMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.EventName = OmniMessageSchema::EventExhausted;
	Message.SetPayloadValue(TEXT("State"), TEXT("True"));
	return Message;
}

bool FOmniExhaustedEventSchema::TryFromMessage(
	const FOmniEventMessage& Message,
	FOmniExhaustedEventSchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.bExhausted = true;
	return true;
}

bool FOmniExhaustedEventSchema::Validate(const FOmniEventMessage& Message, FString& OutError)
{
	if (Message.SourceSystem != OmniMessageSchema::SystemStatus || Message.EventName != OmniMessageSchema::EventExhausted)
	{
		OutError = TEXT("Event schema mismatch for Exhausted.");
		return false;
	}

	FString StateValue;
	if (!Message.TryGetPayloadValue(TEXT("State"), StateValue))
	{
		OutError = TEXT("Missing payload key 'State' for Exhausted event.");
		return false;
	}

	bool bState = false;
	if (!TryParseBool(StateValue, bState) || !bState)
	{
		OutError = TEXT("Invalid payload value 'State' for Exhausted event.");
		return false;
	}

	return true;
}

FOmniEventMessage FOmniExhaustedClearedEventSchema::ToMessage(const FOmniExhaustedClearedEventSchema& Data)
{
	FOmniEventMessage Message;
	Message.SourceSystem = Data.SourceSystem;
	Message.EventName = OmniMessageSchema::EventExhaustedCleared;
	Message.SetPayloadValue(TEXT("State"), TEXT("False"));
	return Message;
}

bool FOmniExhaustedClearedEventSchema::TryFromMessage(
	const FOmniEventMessage& Message,
	FOmniExhaustedClearedEventSchema& OutData,
	FString& OutError
)
{
	if (!Validate(Message, OutError))
	{
		return false;
	}

	OutData.SourceSystem = Message.SourceSystem;
	OutData.bExhausted = false;
	return true;
}

bool FOmniExhaustedClearedEventSchema::Validate(const FOmniEventMessage& Message, FString& OutError)
{
	if (Message.SourceSystem != OmniMessageSchema::SystemStatus || Message.EventName != OmniMessageSchema::EventExhaustedCleared)
	{
		OutError = TEXT("Event schema mismatch for ExhaustedCleared.");
		return false;
	}

	FString StateValue;
	if (!Message.TryGetPayloadValue(TEXT("State"), StateValue))
	{
		OutError = TEXT("Missing payload key 'State' for ExhaustedCleared event.");
		return false;
	}

	bool bState = true;
	if (!TryParseBool(StateValue, bState) || bState)
	{
		OutError = TEXT("Invalid payload value 'State' for ExhaustedCleared event.");
		return false;
	}

	return true;
}

bool FOmniMessageSchemaValidator::ValidateCommand(const FOmniCommandMessage& Message, FString& OutError)
{
	if (Message.TargetSystem == OmniMessageSchema::SystemActionGate && Message.CommandName == OmniMessageSchema::CommandStartAction)
	{
		return FOmniStartActionCommandSchema::Validate(Message, OutError);
	}
	if (Message.TargetSystem == OmniMessageSchema::SystemActionGate && Message.CommandName == OmniMessageSchema::CommandStopAction)
	{
		return FOmniStopActionCommandSchema::Validate(Message, OutError);
	}
	if (Message.TargetSystem == OmniMessageSchema::SystemStatus && Message.CommandName == OmniMessageSchema::CommandSetSprinting)
	{
		return FOmniSetSprintingCommandSchema::Validate(Message, OutError);
	}

	return true;
}

bool FOmniMessageSchemaValidator::ValidateEvent(const FOmniEventMessage& Message, FString& OutError)
{
	if (Message.SourceSystem == OmniMessageSchema::SystemStatus && Message.EventName == OmniMessageSchema::EventExhausted)
	{
		return FOmniExhaustedEventSchema::Validate(Message, OutError);
	}
	if (Message.SourceSystem == OmniMessageSchema::SystemStatus && Message.EventName == OmniMessageSchema::EventExhaustedCleared)
	{
		return FOmniExhaustedClearedEventSchema::Validate(Message, OutError);
	}

	return true;
}

bool FOmniMessageSchemaValidator::ValidateQuery(const FOmniQueryMessage& Message, FString& OutError)
{
	if (Message.TargetSystem == OmniMessageSchema::SystemActionGate && Message.QueryName == OmniMessageSchema::QueryCanStartAction)
	{
		return FOmniCanStartActionQuerySchema::Validate(Message, OutError);
	}
	if (Message.TargetSystem == OmniMessageSchema::SystemStatus && Message.QueryName == OmniMessageSchema::QueryIsExhausted)
	{
		return FOmniIsExhaustedQuerySchema::Validate(Message, OutError);
	}
	if (Message.TargetSystem == OmniMessageSchema::SystemStatus && Message.QueryName == OmniMessageSchema::QueryGetStateTagsCsv)
	{
		return FOmniGetStateTagsCsvQuerySchema::Validate(Message, OutError);
	}

	return true;
}
