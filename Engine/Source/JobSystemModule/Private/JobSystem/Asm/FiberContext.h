#include "JobSystem/FiberContext.h"

#include <cstdint>

using FiberSwitchCallbackFn = void(*)(void* userdata);

extern "C" void FiberGetContext(Volt::FiberContext* context);
extern "C" void FiberSetContext(Volt::FiberContext* context);
extern "C" void FiberSwapContext(Volt::FiberContext* fromContext, Volt::FiberContext* toContext, FiberSwitchCallbackFn callback, void* callbackUserData);
extern "C" void FiberThreadGetFPState(Volt::FiberThreadFPState* state);
