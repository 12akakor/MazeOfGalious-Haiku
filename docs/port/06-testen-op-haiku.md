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

## 5. Pakket

    make package
    pkgman install ./mazeofgalious-0.63.1-2-x86_64.hpkg

Controle: "Maze of Galious" staat in het Deskbar-menu onder Applications (alleen het programma, geen mappen) en start van daaruit in een venster.

Een melding dat het verversen van de HaikuPorts-repository mislukt is, staat los van een lokale installatie en kan geen kwaad.
Verwijderen: `pkgman uninstall mazeofgalious`.

## Bekende aandachtspunten (nog niet getest op echte Haiku)

- Volledig scherm gebruikt een randloos venster ter grootte van het scherm met "floating all" feel (boven de Deskbar). Gedrag in VirtualBox controleren.
- In VirtualBox kan het geluid achterlopen of kraken door de virtuele geluidskaart; vergelijk met andere Haiku-apps.
- `requires { haiku }` in het pakket: op RenkuOS controleren of het systeempakket ook `haiku` heet.
