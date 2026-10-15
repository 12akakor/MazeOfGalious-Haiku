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

- Pakket 0.63.1-1 installeerde, maar de Deskbar toonde naast het programma ook de datamappen; starten via de Deskbar gaf volledig scherm (upstream-standaard). Opgelost in 0.63.1-2: Haiku-indeling (`apps/MazeOfGalious`, `data/MazeOfGalious/`, `documentation/packages/mazeofgalious/`) en standaard een venster. Indeling gecontroleerd met de Haiku `package`-tool.

## Versie 2 (0.63.2, 2026-10-14)

- **xBR-lv2** als CPU-filter (C++-port van Hyllian's shader, MIT), daarna resamplen naar venster of scherm met behoud van 16:10. Ook **sharp bilinear** en **nearest**. Wisselen met **F3**; keuze in `port.cfg`; actief filter in de titelbalk.
- Per grafische set automatisch bepaald of de kunst 320x200 met verdubbelde pixels is (`original`: ja; alle andere sets: nee). xBR werkt dan op 320x200.
- Controle: uitvoer vergeleken met de originele GLSL-shader in WebGL (headless Chromium). MoG-scènes: 95,3 tot 98,2% van de pixels identiek, 99,92 tot 99,99% identiek of 1 stap verschil; testpatroon volledig identiek (hash in unit tests).
- Snelheid (2 cores, Linux, ter indicatie): xbr volledig scherm 1920x1080 ongeveer 20 tot 22 ms per frame, venster 1280x800 ongeveer 5 ms (budget 40 ms bij 25 fps).
- **Extra sets** als apart pakket `mazeofgalious_extras` (graphics naramura, hinox, boltian, alternate, alfonso; geluid Bart, Rob, wolf). packagefs voegt de mappen samen met die van het hoofdpakket. Alle vijf grafische sets geladen en getekend in een testrun.
- Unit tests: 97 checks.

## Versie 2, eerste test op Haiku (2026-10-14)

- `make test` faalde: testhooks-macro `rand()` botste met `<algorithm>`. Opgelost; controlescript compileert nu ook de testvariant.
- Opmerkingen Akakor en oplossing (0.63.2-2):
  - F3 niet uitgelegd: toetsenscherm (K) noemt nu F3 en ALT ENTER; afsluiten staat er als F12 OR ALT Q (was APPLE+Q).
  - F3 gaf geen zichtbaar verschil: het venster was 640x400 (scherm van de VM te klein voor 2x); bij een 640x400-set geven alle filters dan hetzelfde beeld. Het venster opent nu zo groot als past (16:10) en kan gemaximaliseerd worden.
  - Standaard beste sets: naramura (graphics) zit nu in het hoofdpakket; standaard naramura en jorito.
- Golden test blijft 1400/1400 identiek: de test gebruikt een eigen datamap met alleen de set original.

## Versie 3 (0.63.3-1, 2026-10-15)

Gekozen door Akakor uit de verbeterlijst: punt 1, 2, 5, 6 en 8.

- **Meten (F4):** regel in beeld en in de Terminal met filtertijd (gemiddeld en maximum), deel van het beeld dat opnieuw gefilterd is, tekentijd, fps en vsync-status.
- **xBR alleen op veranderde delen:** vorige frame, luma, xBR-resultaat en geschaalde uitvoer worden bewaard; alleen blokken van 16x16 kunstpixels die veranderd zijn (plus twee pixels marge, het bereik van xBR) worden opnieuw berekend. Uitvoer bit-identiek aan volledig filteren, gecontroleerd op alle 1400 frames van de testrun met set original en naramura, op vijf venstergroottes. Snelheid (Linux, 2 cores, 1728x1080): naramura 26,0 naar 2,5 ms gemiddeld per frame, original 12,8 naar 1,8 ms. Bij een kamerwissel blijft één frame duur (tot ongeveer 40 ms).
- **Betere geluidsomzetting:** windowed sinc (Kaiser, 16 nuldoorgangen, afsnijding op 90% van de laagste Nyquist-frequentie) in plaats van lineair. Sinustest 11025 naar 44100 Hz: 83 dB signaal/fout (lineair: 31 dB bij 1 kHz, 8 dB bij 4 kHz).
- **Vsync:** wacht voor het tekenen op de schermverversing via `BScreen::WaitForRetrace()`. In de RenkuOS-bron melden de VESA- en framebuffer-drivers dit niet (geven -1); intel_extreme en radeon_hd wel. In VirtualBox heeft het dus geen effect; de status staat in de F4-regel. Uit te zetten met `vsync=0` in `port.cfg`.
- **Venster onthouden:** grootte, positie en volledig scherm in `port.cfg`; `--window`/`--fullscreen` gaan voor. Een opgeslagen positie buiten het scherm wordt genegeerd.
- Upstream-fout opgelost: `fclose(NULL)` als een PCX-bestand ontbreekt.
- Unit tests: 111 checks. Golden test 1400/1400 identiek. 60 cross-compilaties zonder fouten.

## Publiceren

HaikuPorts accepteert geen AI-bijdragen. Spritz Depot (spritzdepot.desktoponfire.com) indexeert apps van hun auteurs: per app een klein YAML-bestand ("cicheto") met naam, beschrijving en downloadplek, in een eigen git-repository ("bacaro"); geen bouwrecept. De publicatiepagina zelf kon niet automatisch gelezen worden (robots.txt). Nodig dus: een downloadplek voor de .hpkg-bestanden (bijvoorbeeld GitHub-releases) en een git-repository met het YAML-bestand.

## Nog te doen

1. Versie 3 testen in de Haiku-VM: `make test`, checklist punt 15 tot en met 30 in `06-testen-op-haiku.md`.
2. Daarna RenkuOS-VM (zelfde stappen).
3. Bevindingen terugkoppelen; daarna eventuele fixes.

## Bekende beperkingen versie 1

- Eigen sets toevoegen kan via een eigen pakket of door ze in `data/` naast het programma te zetten (ontwikkelmap); een aparte gebruikersmap is er niet.
