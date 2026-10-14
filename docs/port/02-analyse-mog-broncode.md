# 02 Analyse MoG broncode (0.63.1548)

## Omvang

- `sources/`: 35 bestanden, ongeveer 27.000 regels C++ (eigenlijk "C met klassen").
- Data: `graphics/` 5,3 MB (85 PCX), `sound/` 33 MB (109 OGG, 76 WAV), `rooms/` 1,3 MB (311 tekst-maps).
- Build: `build/linux/Makefile` (g++, `sdl-config`), `debian/` pakketbestanden.

## Afhankelijkheden

| Library | Gebruik |
|---|---|
| SDL 1.2 | venster, 8-bit video, events, keyboard state, timer |
| SDL_image | alleen `IMG_Load` in `get_palette()` (palet uit `konami.pcx`) |
| SDL_mixer | SFX: `Mix_LoadWAV`, `Mix_PlayChannel`, `Mix_VolumeChunk`; muziek via `Mix_HookMusic` |
| SDL_sound | muziek decoderen (OGG/WAV) in eigen callback `myMusicPlayer` |

PCX-bitmaps worden met eigen code geladen (`loadpcx.cpp`, `bitmaps.cpp`). Geen OpenGL.

## Architectuur

- **Video:** spel tekent in eigen 8-bit buffer `buffer_screen` (640x400, 1 byte per pixel, palet 256 kleuren). `render.cpp` kopieert/schaalt dit naar de SDL surface (`SDL_SetVideoMode(640,400,8,SDL_HWPALETTE)`). Palet via `SDL_SetColors`. Zoom met PgUp/PgDn via "views" (`logic_*`/`physic_*`).
- **Game loop:** `main.cpp`, vaste tik `REDRAWING_PERIOD = 40` ms (25 fps), frame-skip tot 10, `SDL_Delay(1)`.
- **Input:** polling met `SDL_GetKeyState` + `SDL_PumpEvents` in gamecycle, ingamecycle, shrines, gameinteractions. Arrays tot `SDLK_LAST`. Toetsen worden als SDLK-getallen in `MoG.cfg` bewaard; `SDL_GetKeyName` voor het herdefinitiescherm.
- **Geluid:** abstractie in `sound.h` (`SOUNDT = Mix_Chunk*`, `Sound_create_sound`, `Sound_play`, `Sound_create_music`, enz.). ongeveer 275 aanroepen van `Sound_play`, verspreid over het spel. Extensies geprobeerd: .WAV .OGG .MP3 (er zijn geen MP3's in de data).
- **Bestanden:** `filehandling.cpp` schrijft userdata naar `$HOME/.mog/` (niet-Windows). Data wordt relatief tot de werkmap gelezen (`graphics/`, `sound/`, `rooms/`), sets worden ontdekt met `opendir`.
- **Platform-ifdefs:** `_WIN32` en `__APPLE__` in main, keyboard, path, filehandling, debug, gamecycle.

## SDL-koppeling per bestand

- Zwaar: `main.cpp`, `render.cpp`, `sound.cpp`, `gamecycle.cpp` (veel SDLK-codes, keynames), `gameaux.cpp` (palet).
- Licht (alleen keystate of include): `ingamecycle`, `shrines`, `gameinteractions`, `gametiles`.
- Alleen `#include "SDL_mixer.h"` voor `Sound_play`: `demons`, `gameobjects`, `roomchange`.

## Conclusie voor een native port

Gunstig: het spel rendert al in een eigen 8-bit framebuffer en geluid zit achter een kleine eigen API. Er is dus een smalle platformlaag nodig:

1. video: 640x400 indexbuffer + palet naar scherm,
2. input: keystate-array met SDL-1.2-nummering,
3. audio: SFX-mixer + muziekstream (WAV/OGG decoderen),
4. timer/sleep,
5. paden (data- en settingsmap).

Risico's: `char*` naar string literals (oud C++), mogelijk 64-bit aannames (eerste grep vond geen pointer naar int casts), afhankelijkheid van werkmap, SDL-1.2 keycodes in configbestand.
