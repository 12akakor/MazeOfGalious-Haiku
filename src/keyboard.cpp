#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include "platform/platform.h"

bool IsAltPressed()
{
	plat_pump_events();
	return (plat_modifiers() & PLAT_MOD_ALT) != 0;
} /* IsAltPressed */ 


bool IsAltPressed2()
{
	plat_pump_events();
	return (plat_modifiers() & PLAT_MOD_ALT) != 0;
}
