# Maze of Galious for Haiku / RenkuOS

Native port of the Brain Games remake of *Knightmare II: The Maze of Galious*
(Konami, MSX, 1987) to Haiku and RenkuOS. The game uses only the Be API
(`libbe`, `libmedia`, `libtranslation`).

Based on the upstream nightly `mog-svn-20120228` (game code identical to
0.63.1548). Upstream: https://mog.jorito.net/

## Build and run on Haiku / RenkuOS (x86_64)

    make            # builds ./MazeOfGalious
    make run        # windowed
    make test       # unit tests + 1400-frame comparison with the reference
    make package    # mazeofgalious-0.63.5-2-x86_64.hpkg and
                    # mazeofgalious_extras-0.63.5-2-any.hpkg (extra sets)

Needs the development tools of the system (`gcc`, `make`, `rc`, `xres`,
`mimeset`, `package`; on Haiku: `pkgman install haiku_devel` if missing).

## Keys

Cursor keys move/jump/climb, Space attacks, M fires the secondary weapon,
F1 item screen, F2 pause, F3 scaling filter (xBR, nearest, sharp), F4 timing
display, S screenshot, F10/9 graphic set, F11/0 sound set, PgUp/PgDn zoom,
Esc title screen, Alt+Enter full screen, F12 quit. Keys can be redefined on
the title screen (K). The game starts in a window and remembers its size,
position and full screen state; `--window` and `--fullscreen` on the
command line override that.

Defaults: graphics set naramura, sound set jorito, filter xBR. The window
opens as large as fits on the screen and can be resized or zoomed; the
filters show their difference when the window is larger than 640x400.

Settings (`MoG.cfg`, and `port.cfg` for the filter, window and vertical
sync) are stored in `~/config/settings/MazeOfGalious/`. `vsync=0` in
`port.cfg` turns waiting for the screen refresh off; it only has an effect
with drivers that report the refresh (not with VESA, as in VirtualBox).

S (or the key chosen as the ninth key on the K screen) saves a screenshot
of the picture as shown, filter included and without black borders, as
`MazeOfGalious <date> <time>.png` in the home folder. The key is stored
in `port.cfg` (`screenshot_key`).

The F4 timing display shows per second: filter time per frame (average and
maximum), the share of the picture filtered again (xBR only redoes the parts
that changed), drawing time, frames per second and the vertical sync state.
The same line is printed in the Terminal.
The package installs the executable in `apps/`, the game data in
`data/MazeOfGalious/` and the documentation in
`documentation/packages/mazeofgalious/`.

## Layout

    src/                game code (upstream, minimal changes marked "Port:")
    src/platform/       platform layer: platform.h, portable mixer and decoders,
                        scaler, Haiku back end
    src/third_party/    stb_vorbis (public domain / MIT)
    src/test/           deterministic test hooks (only with -DMOG_TESTHOOKS)
    data/               graphics (original, naramura), sound (original, jorito,
                        alternate), rooms (main package)
    data-extras/        extra graphic and sound sets (extras package)
    tests/              unit tests, input scripts, golden frame hashes
    tools/              test, reference, icon and package scripts
    docs/port/          analysis, plan and decisions (Dutch)

## Licence

Game code and data: GNU GPL v2 (see LICENSE), (c) 2002-2009 Brain Games.
stb_vorbis: public domain or MIT (src/third_party/LICENSE.stb_vorbis.txt).
xBR-lv2 filter: C++ port of Hyllian's shader, MIT (src/third_party/LICENSE.xbr.txt).
Original game (c) 1987 Konami.
