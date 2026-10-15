# 06 Testen op Haiku / RenkuOS

Doel: controleren dat de native build werkt in een VM (VirtualBox, Haiku of RenkuOS x86_64).

## 1. Broncode in de VM krijgen

Kies een van beide:
- **Archief:** kopieer `mog-port-src.zip` naar de VM (gedeelde map, USB-image of `scp`), pak uit: `unzip mog-port-src.zip`.
- **Git:** als de repo op GitHub staat: `git clone <url> mog-port`.

## 2. Ontwikkeltools

In Terminal:

    gcc --version
    which rc xres mimeset package make

Ontbreekt iets: `pkgman install haiku_devel` (gcc en make zitten standaard in Haiku).

## 3. Bouwen en automatisch testen

    cd mog-port
    make
    make test

Verwacht:
- `make` maakt `MazeOfGalious` zonder fouten.
- `make test` toont `NN checks, 0 failures` en `PASS basic: all 1400 frames identical`.
  Tijdens de test opent kort een venster; niet op het toetsenbord drukken.

Wat de test bewijst: dezelfde 1400 beelden en paletten als het originele SDL-spel (gescripte invoer, vaste random-reeks).

Bij `FAIL`: de uitvoermap wordt bewaard (het pad staat in de melding). Stuur de eerste regels van de melding en `stdout.log` uit die map.

## 4. Handmatige controle (checklist)

Start: `make run` (venster) of dubbelklik op `MazeOfGalious` in Tracker.

| # | Controle | Verwacht |
|---|---|---|
| 1 | Opstarten | Konami-logo, titelscherm zonder muziek (zoals upstream); na ongeveer 10 s wachten volgt het verhaal-scherm met muziek |
| 2 | Venster | 1280x800 (of kleiner bij klein scherm), scherp beeld |
| 3 | Spatie | Startdeuntje, spel start, kasteelmuziek |
| 4 | Cursortoetsen, spatie | Lopen, springen, zwaard met geluid |
| 5 | Ladder op/af | Werkt, kamerwissel werkt |
| 6 | F1 / F2 | Itemscherm / pauze |
| 7 | F10, F11 | Wisselt graphics/geluidsset (titelscherm toont naam) |
| 8 | PgUp/PgDn | Zoom |
| 9 | Alt+Enter | Volledig scherm, cursor weg; nogmaals: terug naar venster |
| 10 | Venster sluiten / F12 / Cmd+Q | Spel stopt netjes |
| 11 | Ander venster activeren | Personage stopt met lopen (geen "hangende" toets) |
| 12 | Toetsen herdefiniëren (K op titelscherm) | Namen kloppen, opgeslagen in `~/config/settings/MazeOfGalious/MoG.cfg` |
| 13 | Geluid | Geen haperingen of kraak; meerdere effecten tegelijk |
| 14 | CPU | `ProcessController`: laag gebruik (doel < 15% van één kern) |

Noteer per punt OK of het probleem.

## 4b. Versie 2: beeldfilters en extra sets

| # | Controle | Verwacht |
|---|---|---|
| 15 | Titelbalk en venster | Toont het filter, bijvoorbeeld `Maze of Galious v0.63 [xbr]`; venster groot (ongeveer 90% van de schermbreedte), te vergroten en te maximaliseren |
| 15b | K op titelscherm | Toetsenscherm noemt ook F3 (filter) en ALT ENTER (volledig scherm) |
| 15c | Standaard sets (zonder MoG.cfg) | Graphics naramura, geluid jorito |
| 16 | F3 (meerdere keren) | Wisselt xbr, nearest, sharp; nearest is kleiner met randen, sharp en xbr vullen het venster; verschil xbr is het duidelijkst bij set original; keuze blijft bewaard |
| 17 | xbr, set original | Gladde contouren, geen blokjes |
| 18 | Venster groter maken | Beeld schaalt mee (16:10), zwarte randen alleen waar nodig |
| 19 | Alt+Enter met xbr | Volledig scherm vult de hoogte; spel blijft vloeiend (25 fps) |
| 20 | Met extras-pakket: F10 | Wisselt door original, naramura, hinox, boltian, alternate, alfonso |
| 21 | Met extras-pakket: F11 | Wisselt door alternate, Bart, jorito, original, Rob, wolf |
| 22 | CPU in ProcessController | Bij xbr volledig scherm: noteer het percentage |

## 4c. Versie 3: snelheid, geluid, vsync, venster onthouden

| # | Controle | Verwacht |
|---|---|---|
| 23 | F4 | Gele regel linksboven: filter, deel opnieuw gefilterd, tekenen, fps, vsync. Dezelfde regel elke seconde in de Terminal. Nog eens F4: weg |
| 24 | F4, volledig scherm met xbr, in een kamer | Noteer de regel. Verwacht: filter enkele ms, opnieuw gefilterd een klein percentage, 25 fps |
| 25 | F4, bij wisselen van kamer | Max filter loopt even op (hele beeld opnieuw); geen merkbare hapering |
| 26 | F4 in VirtualBox | `vsync not supported` (VESA-driver meldt de schermverversing niet); geen vertraging |
| 27 | Geluid set original (11025 Hz samples) | Helderder dan versie 2, geen gesis of metalig randje; muziek loopt gelijk, geen tikken bij herhalen |
| 28 | Venster verplaatsen en vergroten, afsluiten, opnieuw starten | Venster terug op dezelfde plek en grootte |
| 29 | Afsluiten in volledig scherm, opnieuw starten | Start in volledig scherm; `--window` op de commandoregel start toch in een venster |
| 30 | K op titelscherm | Toetsenscherm noemt ook F4 |

## 5. Pakket

    make package
    pkgman install ./mazeofgalious-0.63.3-1-x86_64.hpkg
    pkgman install ./mazeofgalious_extras-0.63.3-1-any.hpkg   (optioneel, extra sets)

Controle: "Maze of Galious" staat in het Deskbar-menu onder Applications (alleen het programma, geen mappen) en start van daaruit in een venster.

Een melding dat het verversen van de HaikuPorts-repository mislukt is, staat los van een lokale installatie en kan geen kwaad.
Verwijderen: `pkgman uninstall mazeofgalious`.

## Bekende aandachtspunten (nog niet getest op echte Haiku)

- Volledig scherm gebruikt een randloos venster ter grootte van het scherm met "floating all" feel (boven de Deskbar). Gedrag in VirtualBox controleren.
- In VirtualBox kan het geluid achterlopen of kraken door de virtuele geluidskaart; vergelijk met andere Haiku-apps.
- `requires { haiku }` in het pakket: op RenkuOS controleren of het systeempakket ook `haiku` heet.
