# 03 Stappenplan: Maze of Galious als native RenkuOS binary

Doel: een native RenkuOS (x86_64) applicatie van de MoG-remake die alleen de systeem-kits gebruikt (`libbe`, `libmedia`, eventueel `libgame`), zonder SDL. Geleverd als `.hpkg` met data.

Werkwijze voor Claude Code: voer de fases in volgorde uit, sluit elke fase af met de genoemde controle, schrijf bevindingen in `docs/port/NN-*.md` en commit per stap. Ga niet door naar de volgende fase als de controle faalt.

Uitgangspunt: Claude Code draait op een Linux-host (of de Mac van de gebruiker); RenkuOS draait in een VirtualBox-VM die via SSH bereikbaar is. Bouwen en testen van de native versie gebeurt IN de VM.

---

## Fase 0: Werkruimte en bron vastleggen

1. Maak repo `mog-renkuos/` met `git init`. Kopieer `CLAUDE.md` (zie bestand 04) in de root.
2. Neem `mog-svn-20120228.zip` uit de projectmap (ingekort: alleen original-sets), pak uit in `upstream/nightly-20120228/`.
3. Clone `https://github.com/OSSGames/GAME-SDL-ACTION-Maze_of_Galious` naar `upstream/mirror-0.63.1548/`.
4. `diff -rq` beide; schrijf verschillen (bronbestanden, data, versienummer) in `docs/port/00-bronvergelijking.md`.
5. Kies basis: de nieuwste (verwacht: nightly). Kopieer naar `src/` + `data/` en commit als "import upstream" ZONDER wijzigingen.

Controle: commit bevat onaangepaste upstream; vergelijking gedocumenteerd.

## Fase 1: Referentie-build op Linux (SDL 1.2)

Doel: weten hoe het spel hoort te werken voordat er iets verandert.

1. Installeer `libsdl1.2-dev` (of `libsdl1.2-compat-dev`), `libsdl-image1.2-dev`, `libsdl-mixer1.2-dev`, `libsdl-sound1.2-dev`.
2. Bouw met `build/linux/Makefile`. Los alleen compileerfouten op die door moderne g++ komen (string literals naar `const char*`, ontbrekende includes); elke fix apart committen.
3. Start, speel titel + eerste kamer, maak screenshots (`docs/port/ref/`).
4. Voeg een determinisme-hook toe achter `#ifdef MOG_TESTHOOKS`: vaste `srand`-seed, input afspelen uit bestand, framebuffer dump (640x400 index + palet) na frame N. Deze hook wordt later op RenkuOS hergebruikt voor pixelvergelijking.

Controle: binary draait, referentiedumps van frame 100, 500, 1000 opgeslagen.

## Fase 2: RenkuOS ontwikkelomgeving

1. Download de nightly image van `https://github.com/RenkuOS/Source/releases/tag/nightly` (x86_64). Controleer het formaat (ISO of raw image).
2. VirtualBox (alles via `VBoxManage`, zodat Claude Code het kan scripten):
   - raw image omzetten: `VBoxManage convertfromraw renku.img renku.vdi --format VDI` (bij een ISO: als optische schijf koppelen en installeren op een nieuwe VDI);
   - `VBoxManage createvm --name RenkuOS --ostype Other_64 --register`
   - `VBoxManage modifyvm RenkuOS --memory 4096 --cpus 4 --firmware bios --nic1 nat --nictype1 82540EM --audio-driver default --audio-controller hda --audio-out on --graphicscontroller vboxsvga --vram 64`
   - `VBoxManage storagectl RenkuOS --name SATA --add sata --controller IntelAhci` en `VBoxManage storageattach RenkuOS --storagectl SATA --port 0 --type hdd --medium renku.vdi`
   - SSH doorsturen: `VBoxManage modifyvm RenkuOS --natpf1 "ssh,tcp,,2222,,22"`
   - starten: `VBoxManage startvm RenkuOS --type gui` (headless kan, maar geluid/beeld test je met gui)
   - Controleer of deze instellingen werken; pas aan en documenteer als RenkuOS iets anders vereist (bv. EFI, andere netwerkkaart).
3. In de VM: sshd aanzetten, sleutel plaatsen. Maak daarna een snapshot (`VBoxManage snapshot RenkuOS take schoon`) om altijd terug te kunnen.
4. Controleer en documenteer in `docs/port/02-renkuos-omgeving.md`:
   - `gcc --version`, `uname -a`, `pkgman list-repos`;
   - aanwezigheid van `/boot/system/develop/headers/os/` (Be API headers) en `makefile-engine`;
   - of `pkgman install gcc` / `haiku_devel` nodig is;
   - of HaikuPorts beschikbaar is (alleen informatief; de native port mag er niet van afhangen).
5. Script `tools/vm-build.sh`: rsync `src/` naar VM, `make` via ssh, binary terugkopieren. Script `tools/vm-shot.sh`: start spel via ssh, wacht, `VBoxManage controlvm RenkuOS screenshotpng shot.png` op de host (geen kopieerstap nodig).

Controle: "hello world" BApplication + BWindow compileert en opent in de VM.

## Fase 3: Platformlaag uit het spel trekken (op Linux, met SDL)

Refactor eerst op het bekende platform, zodat regressies meteen zichtbaar zijn.

1. Nieuwe interface `src/platform/platform.h`:
   - `plat_init(int w, int h)`, `plat_shutdown()`
   - `plat_set_palette(const uint8_t rgb[256*3])`
   - `plat_present(const uint8_t* index_buf, int w, int h)` (inclusief zoom-views)
   - `plat_ticks_ms()`, `plat_sleep_ms()`
   - `plat_pump_events()`, `const uint8_t* plat_key_state()`, `const char* plat_key_name(int)`, `bool plat_quit_requested()`, `plat_toggle_fullscreen()`
   - audio: `plat_audio_init()`, `snd_load()`, `snd_play(id, volume)`, `music_play(files[3])`, `music_pause/resume/stop()`, `music_volume()`
   - paden: `plat_data_dir()`, `plat_settings_dir()`
2. Eigen `src/platform/mogkeys.h` met dezelfde numerieke waarden als SDL 1.2 `SDLK_*` (inclusief `MOGK_LAST`), zodat `MoG.cfg` compatibel blijft. Vervang alle `SDLK_*` in de spelcode door `MOGK_*`.
3. Vervang `IMG_Load` in `get_palette()` door de eigen PCX-loader (palet = laatste 769 bytes van PCX, marker 0x0C). Test dat palet identiek is.
4. Schrijf eigen audiodecoders, onafhankelijk van SDL:
   - WAV (PCM 8/16 bit, mono/stereo) parser;
   - OGG via `stb_vorbis.c` (public domain/MIT, als bestand in `src/third_party/`; licentie in README vermelden);
   - lineaire resampler naar 44.100 Hz stereo int16;
   - mixer: 16 SFX-kanalen + 1 muziekstream, volume 0..128 zoals `Mix_VolumeChunk`, gedrag van `Mix_PlayChannel(-1,...)` (eerste vrije kanaal, anders niets).
   - muziek: zelfde intro/loop/eind-logica als `myMusicPlayer` (3 bestanden).
5. Implementeer `platform_sdl.cpp` met SDL alleen voor video/input/audio-uitvoer; mixer en decoders zijn al eigen code.
6. Unit tests (`tests/`, eenvoudige runner, geen framework nodig): WAV-header parsing, ogg decode lengte, mixer clipping, keymap-tabellen, PCX-palet.

Controle: Linux-build met nieuwe laag geeft identieke framebuffer-dumps als fase 1; geluid klinkt gelijk (handmatig + test op samplecount). Geen `SDL` meer buiten `src/platform/`: `grep -rn SDL src --include=*.cpp | grep -v platform/` geeft niets.

## Fase 4: Native RenkuOS backend (`platform_renku.cpp`)

Uitsluitend Be/Haiku API. Stijl volgens Haiku coding guidelines.

1. **Applicatie:** `MoGApp : BApplication` met signature `application/x-vnd.BrainGames-MazeOfGalious`. `ReadyToRun()` start de gamethread met `spawn_thread` (prioriteit `B_DISPLAY_PRIORITY`). `QuitRequested()` zet stopvlag en `wait_for_thread`.
2. **Venster/video:**
   - `MoGWindow : BWindow` (B_TITLED_WINDOW, niet resizebaar of met vaste 640x400 * schaal).
   - `MoGView : BView` met `B_WILL_DRAW`, `SetViewColor(B_TRANSPARENT_COLOR)`.
   - Twee `BBitmap(BRect(0,0,639,399), B_RGB32)` (double buffer). De gamethread zet index+palet om via een 256-entry `uint32` LUT naar de achterbuffer, wisselt onder een `BLocker`, roept `LockLooper()/Invalidate()/UnlockLooper()` aan (of stuurt een bericht).
   - `Draw()`: `DrawBitmap(bitmap, bitmap->Bounds(), Bounds())` voor schaling; nearest neighbour standaard, optioneel `B_FILTER_BITMAP_BILINEAR`.
   - Fullscreen (Alt+Enter): venster naar `BScreen().Frame()`, `B_NO_BORDER_WINDOW_LOOK`, aspect 16:10 behouden met zwarte randen; terugschakelen herstelt frame.
   - Zoom-views uit `render.cpp` blijven in de generieke code; backend krijgt het eindbeeld.
   - Voorbereiding op betere beeldkwaliteit later (zie `05-toekomst-beeldkwaliteit.md`): na de palet-omzetting gaat het RGB32-beeld door een `Scaler`-interface naar een uitvoerbitmap op venstergrootte. Versie 1 levert alleen `NearestScaler` (integer schaling, zwarte randen). Instelling `scaler=nearest` in de settingsfile. Geen OpenGL/Vulkan in versie 1.
3. **Input:**
   - Polling via `get_key_info(&info)` in `plat_pump_events()`, alleen als `window->IsActive()`.
   - Tabel Haiku raw keycode (0x01 Esc, 0x02..0x0D F1..F12, cursors 0x57/0x61/0x62/0x63, enz.) naar `MOGK_*`. Zet de tabel in een apart bestand met test.
   - Toetsnamen via eigen tabel (komt overeen met `SDL_GetKeyName`-teksten).
   - Venster sluiten / Cmd+Q: `B_QUIT_REQUESTED` naar stopvlag. F12 = quit blijft.
4. **Audio:** `BSoundPlayer` met `media_raw_audio_format` 44.100 Hz, 2 kanalen, `B_AUDIO_SHORT`, buffergrootte ongeveer 2048 frames. Callback roept de eigen mixer uit fase 3 aan. Mixer-state beveiligen met een lock-free ringbuffer of korte `BLocker` (nooit langer blokkeren dan nodig in de audiothread). Als er geen audio-device is: stil doorgaan.
5. **Timing:** `system_time()` (microseconden) voor `plat_ticks_ms`, `snooze_until()` om op 40 ms te tikken; de bestaande frame-skip logica blijft.
6. **Paden:**
   - Data: map van de executable (`BApplication::GetAppInfo` naar `BEntry` naar `BPath` parent), daarin `data/`. Alternatief bij hpkg: `find_directory(B_SYSTEM_DATA_DIRECTORY)`/`MazeOfGalious`.
   - Settings: `find_directory(B_USER_SETTINGS_DIRECTORY)` + `MazeOfGalious/` (vervangt `$HOME/.mog`).
   - Lees data niet meer relatief tot de werkmap; zet paden in `path.cpp` om naar absolute paden.
   - Grafische en geluidssets blijven runtime-ontdekt (`opendir` op `graphics/` en `sound/`). Zoek in twee mappen: de pakket-datamap (alleen-lezen) en een gebruikersmap `B_USER_NONPACKAGED_DATA_DIRECTORY/MazeOfGalious/` voor sets die later worden toegevoegd. Een set ontbreekt in de config: terugvallen op `original`.
7. **Build:** `Makefile` afgeleid van de upstream `build/linux/Makefile` (zelfde objectlijst, `-Wno-write-strings`), of op basis van `makefile-engine`: `g++ -O2 -std=gnu++17 -Wall`, `LIBS = be media` (+ `game` alleen als gebruikt), `RDEFS`/resource met app signature, icoon (HVIF) en versie. Geen `-lSDL`.
8. **Resources:** `MazeOfGalious.rdef` met `app_signature`, `app_version`, `app_flags B_SINGLE_LAUNCH`, vector-icoon (eigen ontwerp, geen Konami-logo).

Controle (in de VM):
- `objdump -p mog | grep NEEDED` toont alleen libroot, libbe, libmedia, libstdc++ (geen SDL);
- spel start vanuit Tracker (dubbelklik) en vanuit Terminal;
- framebuffer-dumps uit `MOG_TESTHOOKS` identiek aan Linux-referentie;
- alle toetsen werken, herdefinitie werkt, `MoG.cfg` verschijnt in de settingsmap;
- muziek loopt correct, SFX overlappen, geen hapering bij 25 fps.

## Fase 5: Afronden en verpakken

1. `.PackageInfo` (naam `mazeofgalious`, architecture x86_64, licentie GPL v2, copyrights), map-indeling:
   `apps/MazeOfGalious/MazeOfGalious` + `data/MazeOfGalious/...` + Deskbar-link in `data/deskbar/menu/Applications`.
2. `package create -C build/pkg mazeofgalious-0.63-1-x86_64.hpkg`, installeren met `pkgman install ./...hpkg`, testen na herstart.
3. Controleer datalicenties (Debian dfsg-verschillen) voordat het pakket publiek gaat; documenteer in `docs/port/05-licenties.md`.
4. Eindrapport `docs/port/99-resultaat.md`: wat werkt, bekende problemen, testmethode ("How this was tested").

## Optioneel daarna

- Gamepad via `BJoystick`.
- Extra filters op de `Scaler`-interface (zie `05-toekomst-beeldkwaliteit.md`).
- Port naar x86_gcc2 is NIET het doel (C++17 vereist moderne gcc).

## Besluiten (2026-10-13)

1. OGG-decoder: `stb_vorbis`, meegebundeld in `src/third_party/`. `libvorbisfile` wordt niet gebruikt.
2. Testen: eerst in een VirtualBox-VM met RenkuOS (fase 2). Echte hardware pas daarna.

3. Basisversie: nightly 20120228 (spelcode identiek aan 0.63.1548, zie `00-bronvergelijking.md`). Tekstbestanden bij import naar LF.
4. Geluidssets in het pakket: `original`, `jorito`, `alternate` (jorito en alternate uit de mirror).

## Besluiten (2026-10-14)

5. Grafische sets: versie 1 met `original`. Extra sets zijn alleen data en kunnen later worden toegevoegd (eventueel als apart `.hpkg`, bv. `mazeofgalious_extras`), zonder codewijziging.
6. Beeldkwaliteit: geen SMAA (past niet bij pixel art). Versie 2 krijgt Hyllian's xBR (MIT) op de CPU, met sharp bilinear als tweede stap; GPU pas als RenkuOS echte 3D-versnelling heeft. Versie 1 bereidt alleen de `Scaler`-interface voor, inclusief bronfactor per set (1 of 2). Zie `05-toekomst-beeldkwaliteit.md`.
