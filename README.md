# Maze of Galious for Haiku / RenkuOS

Native port of the Brain Games remake of *Knightmare II: The Maze of Galious*
(Konami, MSX, 1987) to Haiku and RenkuOS. The game uses only the Be API
(`libbe`, `libmedia`); SDL is not needed.

Based on the upstream nightly `mog-svn-20120228` (game code identical to
0.63.1548). Upstream: https://mog.jorito.net/

## Build and run on Haiku / RenkuOS (x86_64)

    make            # builds ./MazeOfGalious
    make run        # windowed
    make test       # unit tests + 1400-frame comparison with the reference
    make package    # mazeofgalious-0.63.1-1-x86_64.hpkg

Needs the development tools of the system (`gcc`, `make`, `rc`, `xres`,
`mimeset`, `package`; on Haiku: `pkgman install haiku_devel` if missing).

## Keys

Cursor keys move/jump/climb, Space attacks, M fires the secondary weapon,
F1 item screen, F2 pause, F10/9 graphic set, F11/0 sound set, PgUp/PgDn zoom,
Esc title screen, Alt+Enter full screen, F12 quit. Keys can be redefined on
the title screen (K). Command line: `--window`, `--fullscreen`.

Settings (`MoG.cfg`) are stored in `~/config/settings/MazeOfGalious/`.

## Layout

    src/                game code (upstream, minimal changes marked "Port:")
    src/platform/       platform layer: platform.h, portable mixer and decoders,
                        scaler, Haiku back end, SDL back end (Linux development)
    src/third_party/    stb_vorbis (public domain / MIT)
    src/test/           deterministic test hooks (only with -DMOG_TESTHOOKS)
    data/               graphics, sound, rooms
    tests/              unit tests, input scripts, golden frame hashes
    tools/              test, reference, icon and package scripts
    docs/port/          analysis, plan and decisions (Dutch)

## Licence

Game code and data: GNU GPL v2 (see LICENSE), (c) 2002-2009 Brain Games.
stb_vorbis: public domain or MIT (src/third_party/LICENSE.stb_vorbis.txt).
Original game (c) 1987 Konami.
