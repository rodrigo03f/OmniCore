#include "Library/OmniMovementLibrary.h"

UOmniDevMovementLibrary::UOmniDevMovementLibrary()
{
	if (Settings.SprintActionId == NAME_None)
	{
		Settings.SprintActionId = TEXT("Movement.Sprint");
	}
}
