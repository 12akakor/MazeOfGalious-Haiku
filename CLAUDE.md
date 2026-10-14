# CLAUDE.md (plaats als `CLAUDE.md` in de root van de repo mog-renkuos)

## Project

Native RenkuOS (Haiku-fork, x86_64) port van de Maze of Galious remake (Brain Games, GPL v2). Doel: binary die alleen de Be/Haiku kits gebruikt (`libbe`, `libmedia`).

Volledig plan: `docs/port/03-stappenplan-claude-code.md`. Werk fase voor fase, stop bij een falende controle.

## Indeling

- `upstream/` onaangepaste bronnen (nooit wijzigen)
- `src/` spelcode; `src/platform/` de ENIGE plek met platformcode
- `src/third_party/` meegeleverde code (stb_vorbis), licenties vermelden
- `data/` graphics, sound, rooms
- `tests/` unit tests, invoerscripts, golden hashes; `tools/` test-, icoon- en pakketscripts
- `docs/port/NN-*.md` bevindingen per fase

## Regels

- Spelgedrag niet veranderen. Refactors moeten pixel-identieke framebuffer-dumps geven (`MOG_TESTHOOKS`).
- Geen SDL of andere externe libraries; alleen de systeem-kits en `src/third_party/stb_vorbis.c`.
- Keycodes `MOGK_*` houden de nummering van het origineel (SDL 1.2-waarden) voor compatibiliteit met bestaande `MoG.cfg`-bestanden.
- Haiku/RenkuOS coding style in `src/platform/platform_haiku.cpp` (tabs, `fMember`, Be-klassen).
- Audiothread nooit lang blokkeren; geen allocaties in de `BSoundPlayer` callback.
- Kleine commits, een logische wijziging per commit. Elke commit compileert.
- Zeg nooit dat iets werkt omdat het compileert: test in de VM en schrijf "How this was tested" in het fasedocument.
- Twijfel over API-gedrag: kijk in `/boot/system/develop/headers/os/` in de VM of in github.com/RenkuOS/Source, niet gokken.

## Commando's (op Haiku/RenkuOS)

- Bouwen: `make`
- Starten: `make run`
- Tests: `make test` (unit tests + 1400 frames vergelijken met `tests/golden`)
- Pakket: `make package`
- Compile-check vanaf een andere machine met cross-compiler: `tools/cross-check.sh`
- Icoon: bron `build/haiku/icon.svg`; `icon2icon build/haiku/icon.svg build/haiku/icon.hvif -f hvif` (hvif-tools) en `tools/hvif2rdef.py build/haiku/icon.hvif > build/haiku/icon.rdef`
- Upstream SDL-referentie (alleen nodig om nieuwe golden hashes te maken): git tag `reference-sdl`
