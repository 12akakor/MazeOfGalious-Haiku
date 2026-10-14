# 07 Voortgang port (2026-10-14)

## Klaar

| Fase | Resultaat | Getest |
|---|---|---|
| 0 Repo | Nightly onaangepast geïmporteerd, regeleinden LF, geluidssets jorito en alternate uit de mirror | |
| 1 Referentie | SDL 1.2 upstream-build met testhooks (gescripte invoer, vaste random-reeks, hash per frame). Git tag `reference-sdl` | 1400 frames deterministisch |
| 3 Platformlaag | Geen SDL meer in de spelcode. Eigen mixer, WAV- en Ogg-decoder (stb_vorbis), PCX-palet, MOGK-toetscodes (SDL 1.2-waarden), `Scaler` met `NearestScaler` | Linux: 1400/1400 frames en paletten identiek aan de referentie; 65 unit tests; geluid bit-identiek aan SDL_mixer voor alle 44,1 kHz-bestanden |
| 4 Native backend | `platform_haiku.cpp`: BWindow/BView/BBitmap, get_key_info, BSoundPlayer, find_directory, volledig scherm; Makefile, rdef, eigen vectoricoon, hpkg-script | Compileert voor Haiku x86_64 (GCC 13.3.0, RenkuOS-headers 13-10). Nog niet gelinkt of gestart |

## Upstream-bug gerepareerd

Standaardinstellingen lazen `g_paths[4]` en `s_paths[2]` (vaste posities). Met minder sets dan verwacht las het spel buiten de array. Nu op naam gekozen, met `original` als terugval. Setlijsten worden gesorteerd (volgorde van `readdir` verschilt per bestandssysteem).

## Nog te doen

1. Testen in de Haiku-VM volgens `06-testen-op-haiku.md` (`make`, `make test`, checklist, `make package`).
2. Daarna RenkuOS-VM (zelfde stappen).
3. Bevindingen terugkoppelen; daarna eventuele fixes.

## Bekende beperkingen versie 1

- Volledig scherm schaalt alleen met een geheel getal. Bij 1024x768 blijft het beeld 640x400 met zwarte randen. xBR (versie 2) lost dit op.
- Grafische sets: alleen `original` in de repo.
- Extra sets in een gebruikersmap (naast de pakketmap) zijn nog niet geïmplementeerd.
