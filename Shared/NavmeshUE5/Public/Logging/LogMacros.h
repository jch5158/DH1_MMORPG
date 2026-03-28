#pragma once
// UE5 Logging/LogMacros.h stub for standalone server build
// UE_LOG and log category macros are already defined in CoreMinimal.h
#ifndef UE_LOG
#include "CoreMinimal.h"
#endif

#ifndef UE_CLOG
#define UE_CLOG(Condition, CategoryName, Verbosity, Format, ...) ((void)0)
#endif
