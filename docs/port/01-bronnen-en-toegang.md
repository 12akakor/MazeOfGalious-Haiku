# 01 Bronnen en toegang

Datum analyse: 2026-10-13

## Nightlies (braingames.jorito.net/nightlies)

- Meest recente MoG nightly: `mog-svn-20120228.zip` (35.548.288 bytes).
- Alle nightlies zijn SVN-exports uit 2011 tot maart 2012, op 22-05-2019 op de server gezet.
- Let op: er staat ook een reeks `mog2-svn-*` (laatste `mog2-svn-20120305.zip`, ongeveer 7 MB). Wat dat precies is (Maze of Galious 2 / herbouw?) is niet gecontroleerd.

## Download geblokkeerd

De cloud-werkomgeving mag `braingames.jorito.net` niet benaderen (proxy geeft 403, beleidsweigering). De zip is daarom NIET zelf gedownload.

Vervanging voor de analyse: GitHub-mirror
`https://github.com/OSSGames/GAME-SDL-ACTION-Maze_of_Galious` (commit `4751f21`, "0.63.1548", 2019-04-14).
Dit komt overeen met de officiele release `mog.src_0.63-1548.tgz` (2009).

**Open punt:** de nightly van 2012-02-28 kan nieuwer zijn dan 0.63.1548. Stap 0 van het stappenplan laat Claude Code de nightly lokaal downloaden en een `diff -r` maken tegen de mirror.

## Infopagina (mog.jorito.net)

- Remake van Konami's *Knightmare II: The Maze of Galious* (MSX, 1987).
- Code: Santi Ontanon, Jorrith Schaap. Remake (c) 2002 tot 2009 Brain Games.
- Platforms: Windows, Mac OS X, Linux (Debian/Ubuntu .deb), broncode in de Linux-download.
- Laatste nieuws: 15-03-2009. Sinds 17-04-2008 weer SDL_sound voor goed loopende muziek.
- Meerdere grafische sets (Naramura, Hinox, Boltian, Alternate, Original) en geluidssets, wisselbaar met F10/F11.
- Licentie code: GPL v2 (LICENSE in repo). Debian verspreidt een `.dfsg`-versie (0.62.dfsg2), wat erop wijst dat niet alle data vrij herdistribueerbaar is. Niet verder uitgezocht.

## RenkuOS

- Website: https://renkuos.org , broncode: https://github.com/RenkuOS/Source
- Onafhankelijke fork van Haiku, pre-release, alleen nightly images (GitHub release tag `nightly`).
- Architecturen genoemd: x86_64 (en x86_gcc2 op de software-pagina).
- Software via HaikuDepot / `pkgman`; extra catalogus "Spritz Depot":
  `pkgman add-repo https://spritzdepot.desktoponfire.com/repo/x86_64/current`
- Erft Haiku coding style en API (BeOS-lijn). Haiku-commits worden selectief gecherry-pickt (UPSTREAM.md).
- Image bevat volgens `build/jam/repositories/Renku` het `makefile_engine` pakket en `haiku_devel`.
- **Onzeker:** of HaikuPorts standaard als repository op een RenkuOS-image staat (de jam-file zegt alleen dat de upstream *Haiku* repository niet geconfigureerd is). Of SDL / gcc direct installeerbaar zijn is niet bevestigd. Wordt in stap 1 gecontroleerd.
