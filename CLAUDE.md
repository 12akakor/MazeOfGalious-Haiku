# CLAUDE.md (plaats als `CLAUDE.md` in de root van de repo mog-renkuos)

## Project

Native RenkuOS (Haiku-fork, x86_64) port van de Maze of Galious remake (Brain Games, GPL v2). Doel: binary zonder SDL, alleen Be/Haiku kits (`libbe`, `libmedia`).

Volledig plan: `docs/port/03-stappenplan-claude-code.md`. Werk fase voor fase, stop bij een falende controle.

## Indeling

- `upstream/` onaangepaste bronnen (nooit wijzigen)
- `src/` spelcode; `src/platform/` de ENIGE plek met platformcode
- `src/third_party/` meegeleverde code (stb_vorbis), licenties vermelden
- `data/` graphics, sound, rooms
- `tests/` unit tests; `tools/` VM build/screenshot scripts
- `docs/port/NN-*.md` bevindingen per fase

## Regels

- Spelgedrag niet veranderen. Refactors moeten pixel-identieke framebuffer-dumps geven (`MOG_TESTHOOKS`).
- Geen SDL buiten `src/platform/platform_sdl.cpp`; de RenkuOS-build linkt nooit tegen SDL.
- Keycodes intern in SDL-1.2-nummering (`MOGK_*`) voor compatibiliteit met `MoG.cfg`.
- Haiku/RenkuOS coding style in `src/platform/platform_renku.cpp` (tabs, `fMember`, Be-klassen).
- Audiothread nooit lang blokkeren; geen allocaties in de `BSoundPlayer` callback.
- Kleine commits, een logische wijziging per commit. Elke commit compileert.
- Zeg nooit dat iets werkt omdat het compileert: test in de VM en schrijf "How this was tested" in het fasedocument.
- Twijfel over API-gedrag: kijk in `/boot/system/develop/headers/os/` in de VM of in github.com/RenkuOS/Source, niet gokken.

## Commando's

- Linux referentie: `make -f build/linux/Makefile`
- RenkuOS build: `tools/vm-build.sh` (rsync + ssh `make` in de VirtualBox-VM, poort 2222)
- Screenshot: `tools/vm-shot.sh`
- Tests: `make test`
