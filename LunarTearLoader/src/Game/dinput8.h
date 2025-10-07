#include "Game/Globals.h"


inline void* GetEnumDevicesAddress() {

	if (!iDirectInput8Ptr) return nullptr;

	void* iDirectInput8 = *(void**)iDirectInput8Ptr;

	if (!iDirectInput8) return nullptr;

	void** vtable = *(void***)iDirectInput8;

	if (!vtable) return nullptr;

	return vtable[4];
}