#define MOG_TESTHOOKS_IMPL
#include "testhooks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

struct Event { int frame, key, down; };

bool sInit = false;
bool sActive = false;
int sFrame = 0;
int sMaxFrames = 0;
Event* sEvents = NULL;
int sEventCount = 0;
int sNextEvent = 0;
int sDumps[64];
int sDumpCount = 0;
char sOut[512] = ".";
unsigned char sHeld[1024];
unsigned int sSeed = 12345;

void
init()
{
	if (sInit)
		return;
	sInit = true;
	memset(sHeld, 0, sizeof(sHeld));
	const char* frames = getenv("MOG_TEST_FRAMES");
	if (frames == NULL)
		return;
	sActive = true;
	sMaxFrames = atoi(frames);
	const char* out = getenv("MOG_TEST_OUT");
	if (out != NULL)
		snprintf(sOut, sizeof(sOut), "%s", out);
	const char* dumps = getenv("MOG_TEST_DUMPS");
	while (dumps != NULL && *dumps != '\0' && sDumpCount < 64) {
		sDumps[sDumpCount++] = atoi(dumps);
		dumps = strchr(dumps, ',');
		if (dumps != NULL)
			dumps++;
	}
	const char* script = getenv("MOG_TEST_SCRIPT");
	if (script != NULL) {
		FILE* fp = fopen(script, "r");
		if (fp != NULL) {
			int capacity = 256;
			sEvents = (Event*)malloc(sizeof(Event) * capacity);
			Event e;
			char line[128];
			while (fgets(line, sizeof(line), fp) != NULL) {
				if (line[0] == '#')
					continue;
				if (sscanf(line, "%d %d %d", &e.frame, &e.key, &e.down) != 3)
					continue;
				if (sEventCount == capacity) {
					capacity *= 2;
					sEvents = (Event*)realloc(sEvents, sizeof(Event) * capacity);
				}
				sEvents[sEventCount++] = e;
			}
			fclose(fp);
		}
	}
}

}	// namespace


extern "C" int
th_active(void)
{
	init();
	return sActive ? 1 : 0;
}


extern "C" int
th_frame(void)
{
	return sFrame;
}


extern "C" void
th_apply_input(unsigned char* keystate, int count)
{
	init();
	if (!sActive || keystate == NULL)
		return;
	while (sNextEvent < sEventCount && sEvents[sNextEvent].frame <= sFrame) {
		Event& e = sEvents[sNextEvent++];
		if (e.key >= 0 && e.key < 1024)
			sHeld[e.key] = e.down ? 1 : 0;
	}
	for (int i = 0; i < count && i < 1024; i++)
		keystate[i] = sHeld[i];
}


extern "C" int
th_end_frame(const unsigned char* buffer, int w, int h,
	const unsigned char* palette)
{
	init();
	if (!sActive)
		return 0;
	{
		/* FNV-1a 64 of every frame (image and palette) for exact comparison. */
		unsigned long long hi = 1469598103934665603ULL;
		unsigned long long hp = 1469598103934665603ULL;
		for (int i = 0; i < w * h; i++)
			hi = (hi ^ buffer[i]) * 1099511628211ULL;
		for (int i = 0; i < 768; i++)
			hp = (hp ^ palette[i]) * 1099511628211ULL;
		char name[600];
		snprintf(name, sizeof(name), "%s/hashes.txt", sOut);
		FILE* fp = fopen(name, sFrame == 0 ? "w" : "a");
		if (fp != NULL) {
			fprintf(fp, "%05d %016llx %016llx\n", sFrame, hi, hp);
			fclose(fp);
		}
	}
	for (int i = 0; i < sDumpCount; i++) {
		if (sDumps[i] != sFrame)
			continue;
		char name[600];
		snprintf(name, sizeof(name), "%s/frame_%05d.raw", sOut, sFrame);
		FILE* fp = fopen(name, "wb");
		if (fp != NULL) {
			fwrite(buffer, 1, w * h, fp);
			fclose(fp);
		}
		snprintf(name, sizeof(name), "%s/frame_%05d.pal", sOut, sFrame);
		fp = fopen(name, "wb");
		if (fp != NULL) {
			fwrite(palette, 1, 768, fp);
			fclose(fp);
		}
	}
	sFrame++;
	return sFrame >= sMaxFrames ? 1 : 0;
}


extern "C" int
th_rand(void)
{
	/* Simple LCG, identical on all platforms. */
	sSeed = sSeed * 1103515245u + 12345u;
	return (int)((sSeed >> 16) & 0x7fff);
}
