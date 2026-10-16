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

## Versie 3, eerste test op Haiku (2026-10-16)

- Spel draait; met xbr hapert de muziek af en toe en "springt" het poppetje soms een stukje bij rennen. Meting met F4 volgt.
- Crash na het Konami-logo bij starten vanuit `~/mog-port`: `MoG.cfg` noemde een set uit het extras-pakket die in `~/mog-port/data` niet staat. De upstream-code liet dan `g_path`/`s_path` leeg (NULL). Opgelost in 0.63.3-2: terugval op de standaardset, toetsinstellingen blijven behouden. Nagespeeld en getest op Linux (AddressSanitizer) met vier combinaties.

- F4-meting (VM met 1 processor, i5-6300U): xBR gemiddeld 3 tot 8 ms per beeld, 0 tot 11% opnieuw gefilterd, 25 fps. Pieken van 52 tot 72 ms bij een volledig nieuw beeld (kamerwissel): langer dan de 40 ms van een speltik, dus het spel haalt in met twee tikken achter elkaar (het "sprongetje"). Sharp: gemiddeld 6 ms, maximum meestal onder 25 ms.
- Tweede oorzaak voor haperende muziek gevonden in de code: bij een muziekwissel werden de Ogg-bestanden geopend terwijl het audio-slot vastgehouden werd; de geluidsthread moest daarop wachten.

## 0.63.4-1 (2026-10-16)

- **xBR met tijdbudget:** per beeld hooguit ongeveer 10 ms xBR-werk. Wat bij een groot verschil (kamerwissel) niet past, toont eerst sharp bilinear en wordt in de volgende beelden afgemaakt (meestal 3 tot 7 beelden, ongeveer 0,1 tot 0,3 s). Daarna is het beeld bit-identiek aan volledig filteren (gecontroleerd op de 1400 testbeelden, beide sets). Hier op 1 thread: maximum per beeld van 52 tot 136 ms naar ongeveer 30 ms.
- **Muziek openen en sluiten buiten het audio-slot.** Geluidsuitvoer van de testrun is byte-identiek aan de vorige versie.
- Unit tests: 115 checks.

- Getest op Haiku (VM 1 processor, 2026-10-16): 416 seconden met xBR, elke seconde 25 fps; hoogste filtertijd 25,4 ms (was 52 tot 72 ms), tekenen hoogstens 11,7 ms. Spel loopt vloeiend volgens Akakor; muziek hapert niet meer.

## 0.63.5-1 (2026-10-16): screenshot

- Op verzoek van Akakor: toets S (standaard) bewaart wat de speler ziet: het beeld met filter, zonder zwarte randen, als PNG in de home-map (`MazeOfGalious <datum> <tijd>.png`, MIME `image/png`). Opslaan gebeurt in een eigen thread; melding onderin beeld en in de Terminal.
- De toets is de negende vraag in het toetsenscherm (K) en wordt bewaard in `port.cfg` (`screenshot_key`), zodat `MoG.cfg` het upstream-formaat houdt. In 0.63.5-2 staan de vragen op dat scherm op opeenvolgende regels (upstream: om de regel), op verzoek van Akakor; de hulpregels staan eronder na een lege regel.
- Niet actief tijdens toetsen herdefiniëren en wachtwoord typen.
- PrtScr kan niet: die toets vangt Haiku zelf af (BWindow, `src/kits/interface/Window.cpp`) voor de systeem-screenshot.
- Extra systeem-kit: `libtranslation` (PNG via de Translation Kit).

## GitHub (2026-10-16)

- Gepubliceerd op https://github.com/12akakor/MazeOfGalious-Haiku (branch main) met Engelse README en screenshots. Commit-e-mail vervangen door het GitHub-noreply-adres (hele geschiedenis herschreven).
- De tag `reference-sdl` kon vanuit deze sessie niet gepusht worden; die is alleen nodig om de golden hashes opnieuw te maken.

## Publiceren

HaikuPorts accepteert geen AI-bijdragen. Spritz Depot (spritzdepot.desktoponfire.com) indexeert apps van hun auteurs: per app een klein YAML-bestand ("cicheto") met naam, beschrijving en downloadplek, in een eigen git-repository ("bacaro"); geen bouwrecept. De publicatiepagina zelf kon niet automatisch gelezen worden (robots.txt). Nodig dus: een downloadplek voor de .hpkg-bestanden (bijvoorbeeld GitHub-releases) en een git-repository met het YAML-bestand.

## Nog te doen

1. Testen in een RenkuOS-VM (zelfde stappen als op Haiku).
2. Bevindingen terugkoppelen; daarna eventuele fixes.

## Bekende beperkingen versie 1

- Eigen sets toevoegen kan via een eigen pakket of door ze in `data/` naast het programma te zetten (ontwikkelmap); een aparte gebruikersmap is er niet.
