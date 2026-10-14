/* Deterministic test hooks (compiled only with -DMOG_TESTHOOKS).
 *
 * Runtime control via environment variables:
 *   MOG_TEST_SCRIPT  input script: lines "<frame> <keycode> <0|1>" (keycode = MOGK number)
 *   MOG_TEST_DUMPS   comma separated frame numbers to dump, e.g. "50,200,600"
 *   MOG_TEST_OUT     directory for dumps (frame_NNNNN.raw = 640x400 index bytes, .pal = 768 bytes RGB)
 *   MOG_TEST_FRAMES  number of frames to run before exiting
 */
#ifndef MOG_TESTHOOKS_H
#define MOG_TESTHOOKS_H

#ifdef __cplusplus
extern "C" {
#endif

int  th_active(void);
int  th_frame(void);
/* Overwrites keystate[0..count) with the scripted key state for this frame. */
void th_apply_input(unsigned char* keystate, int count);
/* palette: 256*3 bytes RGB. Returns nonzero when the run is complete. */
int  th_end_frame(const unsigned char* buffer, int w, int h, const unsigned char* palette);
int  th_rand(void);
/* Renders one frame (1/25 s) of audio and appends it to MOG_TEST_OUT/audio.raw
   (44100 Hz stereo int16 little endian). */
void th_audio_frame(void);

#ifdef __cplusplus
}
#endif

#if defined(MOG_TESTHOOKS) && !defined(MOG_TESTHOOKS_IMPL)
/* Same random sequence on every platform. The standard headers that use
   rand() themselves are included first, so the macro does not reach them. */
#include <stdlib.h>
#ifdef __cplusplus
#include <cstdlib>
#include <algorithm>
#endif
#define rand() th_rand()
#endif

#endif
