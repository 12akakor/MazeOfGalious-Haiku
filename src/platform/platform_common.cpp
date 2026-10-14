/*
 * Platform code shared by all back ends: key state, event queue, palette.
 */
#include "platform_internal.h"
#include "video_scaler.h"

#include <mutex>
#include <string.h>


namespace {

std::mutex sLock;
unsigned char sKeys[MOGK_LAST];
bool sOverride = false;

const int kQueueSize = 64;
PlatEvent sQueue[kQueueSize];
int sQueueHead = 0;
int sQueueCount = 0;

uint8_t sPalette[768];
Scaler* sScaler = NULL;
int sSourceFactor = 1;
uint32_t sLut[256];

}	// namespace


void
plat_internal_set_key(MOGKey key, bool down)
{
	if (key <= 0 || key >= MOGK_LAST || sOverride)
		return;
	sKeys[key] = down ? 1 : 0;
}


void
plat_internal_clear_keys(void)
{
	if (!sOverride)
		memset(sKeys, 0, sizeof(sKeys));
}


void
plat_internal_push_event(int type, MOGKey key, int modifiers)
{
	std::lock_guard<std::mutex> guard(sLock);
	if (sQueueCount == kQueueSize)
		return;
	PlatEvent& event = sQueue[(sQueueHead + sQueueCount) % kQueueSize];
	event.type = type;
	event.key = key;
	event.modifiers = modifiers;
	sQueueCount++;
}


bool
plat_internal_pop_event(PlatEvent* event)
{
	std::lock_guard<std::mutex> guard(sLock);
	if (sQueueCount == 0)
		return false;
	*event = sQueue[sQueueHead];
	sQueueHead = (sQueueHead + 1) % kQueueSize;
	sQueueCount--;
	return true;
}


bool
plat_internal_input_override(void)
{
	return sOverride;
}


void
plat_set_input_override(bool enabled)
{
	sOverride = enabled;
}


unsigned char*
plat_key_state(void)
{
	return sKeys;
}


void
plat_set_palette(const uint8_t* rgb)
{
	memcpy(sPalette, rgb, sizeof(sPalette));
	for (int i = 0; i < 256; i++) {
		sLut[i] = 0xff000000u | ((uint32_t)rgb[i * 3] << 16)
			| ((uint32_t)rgb[i * 3 + 1] << 8) | rgb[i * 3 + 2];
	}
}


void
plat_get_palette(uint8_t* rgb)
{
	memcpy(rgb, sPalette, sizeof(sPalette));
}


void
plat_internal_convert(const uint8_t* src, uint32_t* dst, int count)
{
	for (int i = 0; i < count; i++)
		dst[i] = sLut[src[i]];
}


int
plat_modifiers(void)
{
	int modifiers = 0;
	if (sKeys[MOGK_LSHIFT] || sKeys[MOGK_RSHIFT])
		modifiers |= PLAT_MOD_SHIFT;
	if (sKeys[MOGK_LCTRL] || sKeys[MOGK_RCTRL])
		modifiers |= PLAT_MOD_CTRL;
	if (sKeys[MOGK_LALT] || sKeys[MOGK_RALT])
		modifiers |= PLAT_MOD_ALT;
	if (sKeys[MOGK_LMETA] || sKeys[MOGK_RMETA] || sKeys[MOGK_LSUPER]
		|| sKeys[MOGK_RSUPER])
		modifiers |= PLAT_MOD_META;
	return modifiers;
}


void
plat_set_scaler(const char* name)
{
	sScaler = scaler_for_name(name);
	plat_backend_scaler_changed();
}


void
plat_next_scaler(void)
{
	sScaler = scaler_next(plat_internal_scaler());
	plat_backend_scaler_changed();
}


const char*
plat_scaler_name(void)
{
	return plat_internal_scaler()->Name();
}


Scaler*
plat_internal_scaler(void)
{
	if (sScaler == NULL)
		sScaler = scaler_for_name("xbr");
	return sScaler;
}


void
plat_set_source_factor(int factor)
{
	sSourceFactor = factor == 2 ? 2 : 1;
}


int
plat_internal_source_factor(void)
{
	return sSourceFactor;
}
