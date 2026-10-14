/*
 * Shared helpers for the platform back ends (not used by the game).
 */
#ifndef MOG_PLATFORM_INTERNAL_H
#define MOG_PLATFORM_INTERNAL_H

#include "platform.h"

/* Key state and events. Thread safe. */
void plat_internal_set_key(MOGKey key, bool down);
void plat_internal_clear_keys(void);
void plat_internal_push_event(int type, MOGKey key, int modifiers);
bool plat_internal_pop_event(PlatEvent* event);
bool plat_internal_input_override(void);

/* Palette -> 32 bit pixels (0xffRRGGBB, matches B_RGB32 on little endian). */
void plat_internal_convert(const uint8_t* src, uint32_t* dst, int count);

#endif
