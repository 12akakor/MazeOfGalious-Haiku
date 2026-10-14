# 07 Voortgang port (2026-10-14)

## Klaar

| Fase | Resultaat | Getest |
|---|---|---|
| 0 Repo | Nightly onaangepast geïmporteerd, regeleinden LF, geluidssets jorito en alternate uit de mirror | |
| 1 Referentie | SDL 1.2 upstream-build met testhooks (gescripte invoer, vaste random-reeks, hash per frame). Git tag `reference-sdl` | 1400 frames deterministisch |
| 3 Platformlaag | Geen SDL meer in de spelcode. Eigen mixer, WAV- en Ogg-decoder (stb_vorbis), PCX-palet, MOGK-toetscodes (SDL 1.2-waarden), `Scaler` met `NearestScaler` | Linux: 1400/1400 frames en paletten identiek aan de referentie; 65 unit tests; geluid bit-identiek aan SDL_mixer voor alle 44,1 kHz-bestanden |
| 4 Native backend | `platform_haiku.cpp`: BWindow/BView/BBitmap, get_key_info, BSoundPlayer, find_directory, volledig scherm; Makefile, rdef, eigen vectoricoon, hpkg-script | Compileert voor Haiku x86_64 (GCC 13.3.0, RenkuOS-headers 13-10). Nog niet gelinkt of gestart |

## Opgeruimd (2026-10-14)

Op verzoek van Akakor zijn alle Linux/SDL-hulpmiddelen uit de repo gehaald: `platform_sdl.cpp`, `Makefile.linux`, `build/linux/`, `tests/unit/audio_compare.cpp` en `tools/build-reference.sh`. De repo bevat nu alleen de native Haiku/RenkuOS-build. De vergelijkingstest met 1400 frames draait native met `make test`; de golden hashes staan vast in `tests/golden`. De SDL-referentie blijft terug te halen via git tag `reference-sdl`.

## Upstream-bug gerepareerd

Standaardinstellingen lazen `g_paths[4]` en `s_paths[2]` (vaste posities). Met minder sets dan verwacht las het spel buiten de array. Nu op naam gekozen, met `original` als terugval. Setlijsten worden gesorteerd (volgorde van `readdir` verschilt per bestandssysteem).

## Getest op Haiku (2026-10-14, VirtualBox op Windows 10)

- `make`: gelukt, gelinkt met alleen `-lbe -lmedia`, icoon toegevoegd.
- `make test`: 65 checks, 0 failures; **PASS basic: all 1400 frames identical**. De native Haiku-build geeft dezelfde 1400 beelden en paletten als het originele SDL-spel.
- Gevonden en opgelost: scripts verloren hun uitvoerrechten bij het uitpakken (nu via `sh`); drie compilerwaarschuwingen uit de upstream-code.

- Handmatige checklist (`make run`): alle punten OK. Het titelscherm heeft geen muziek; dat is ook zo in de upstream-code (muziek start pas na spatie), dus geen fout van de port.

## Nog te doen

1. `make package` en installatie testen in de Haiku-VM.
2. Daarna RenkuOS-VM (zelfde stappen).
3. Bevindingen terugkoppelen; daarna eventuele fixes.

## Bekende beperkingen versie 1

- Volledig scherm schaalt alleen met een geheel getal. Bij 1024x768 blijft het beeld 640x400 met zwarte randen. xBR (versie 2) lost dit op.
- Grafische sets: alleen `original` in de repo.
- Extra sets in een gebruikersmap (naast de pakketmap) zijn nog niet geïmplementeerd.
