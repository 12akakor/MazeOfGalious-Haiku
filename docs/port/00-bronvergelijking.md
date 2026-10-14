# 00 Bronvergelijking: nightly 20120228 vs mirror 0.63.1548

Datum: 2026-10-14

Vergeleken:
- **Nightly:** `mog-svn-20120228.zip` (ingekorte versie van Akakor: alleen `graphics/original` en `sound/original`, 487 bestanden).
- **Mirror:** github.com/OSSGames/GAME-SDL-ACTION-Maze_of_Galious, commit `4751f21` ("0.63.1548").

Methode: `diff -rq --strip-trailing-cr` (de nightly heeft CRLF-regeleinden in tekst- en mapbestanden, de mirror LF).

## Resultaat

| Onderdeel | Uitkomst |
|---|---|
| `sources/` (35 bestanden) | **Identiek** (alleen regeleinden verschillen) |
| `rooms/` (311 maps) | Identiek (alleen regeleinden) |
| `graphics/original/` (28 bestanden) | Identiek |
| `sound/original/` (86 bestanden) | Identiek |
| readme/leeme/MoG-FAQ/LICENSE | Identiek (alleen regeleinden) |
| `build/linux/Makefile` | Mirror heeft `-Wno-write-strings` en een startscript met `#!/bin/sh`; verder gelijk |
| `build/linux/INSTALL` | Mirror noemt SDL_sound als afhankelijkheid (nightly-INSTALL vergeet die, de Makefile linkt hem wel) |
| Alleen in nightly | `build/osx/` (Xcode-project, SDLMain), `build/windows/` (Dev-C++ project, .rc/.ico) |
| Alleen in mirror | `debian/`, `README.md`, `liesmich.txt`, lege `INSTALL`/`Makefile` in root, extra grafische sets (alfonso, alternate, boltian, hinox, naramura) en geluidssets (Bart, Rob, alternate, jorito, wolf) |

## Conclusie

- De spelcode is tussen 0.63.1548 (2009) en de nightly van 2012-02-28 **niet veranderd**. De analyse in `02-analyse-mog-broncode.md` geldt dus volledig voor de nightly.
- **Basis voor de port:** de nightly (authentieke upstream). Bij import alle tekstbestanden naar LF omzetten (aparte commit).
- Extra geluidssets `jorito` en `alternate` (wens Akakor: original, jorito, alternate) komen niet uit de ingekorte nightly; die worden uit de mirror overgenomen. Herkomst vermelden in `docs/port/05-licenties.md`.
- `build/osx` en `build/windows` zijn voor de port niet nodig; alleen ter referentie in `upstream/` bewaren.
