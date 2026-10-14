# 05 Toekomst: betere beeldkwaliteit (na versie 1)

Datum: 2026-10-14. Advies, nog geen besluit.

## SMAA past niet goed bij dit spel

SMAA is anti-aliasing voor 3D-beelden: het verzacht trapjes aan schuine randen van polygonen. MoG is 2D pixel art met een palet van 256 kleuren. Er zijn geen polygoonranden; de "trapjes" zijn de bedoelde pixels. SMAA op pixel art geeft vooral wat vervaging en weinig winst.

Wat wel werkt voor pixel art:

| Techniek | Effect | Licentie (te controleren) |
|---|---|---|
| Integer schaling + "sharp bilinear" | scherp, geen vervorming bij elke venstergrootte | eigen code |
| xBR / ScaleFX (Hyllian) | gladde contouren, behoudt pixel-karakter | shaders MIT |
| HQ2x/3x/4x | klassiek, iets zachter | LGPL 2.1 |
| xBRZ (Zenju, CPU C++) | zeer goed, multithreaded | naar mijn weten GPL v3: mogelijk conflict met MoG (GPL v2, geen "or later" in de bronbestanden) |
| CRT-shader (scanlines, maskers) | MSX-gevoel | meestal GPL/MIT, per shader |

## CPU of GPU (BGLView of Vulkan)

- In VirtualBox heeft een Haiku/RenkuOS-gast geen 3D-versnelling. OpenGL via `BGLView` draait daar op Mesa software rendering (llvmpipe), dus op de CPU.
- Op echte hardware: voor zover bekend doen Haiku's standaard AMD- en Intel-drivers alleen modesetting, zonder 3D. Vulkan bestaat alleen als experimenteel werk van X512 buiten Haiku zelf: RADV voor Radeon (sinds 2021) en een alpha NVIDIA-driver met NVK (januari 2026). Niet iets om een release op te bouwen. Of RenkuOS dit overneemt is onbekend.
- Gevolg: een filter op de CPU is nu de enige route die overal werkt. Bij 640x400 invoer is dat goed haalbaar (xBR-achtige filters zijn per pixel goedkoop, kan over meerdere threads).

Advies volgorde:
1. **Versie 1:** `BBitmap` + `DrawBitmap`, nearest neighbour, integer schaling, zwarte randen.
2. **Versie 2:** CPU-filter (sharp bilinear, daarna xBR of HQx) in eigen code, multithreaded.
3. **Later, optioneel:** OpenGL-backend met `BGLView` en dezelfde filters als GLSL-shader. Alleen zinvol als RenkuOS echte 3D-versnelling krijgt. Vulkan pas als dat in RenkuOS standaard en stabiel is.

## Wat versie 1 nu al moet voorbereiden

1. **Scheiding logica en weergave:** het spel levert per frame alleen een 640x400 indexbuffer + palet (zit al in het ontwerp).
2. **Filterstap als interface** tussen palet-omzetting en tonen:
   `class Scaler { virtual void Process(const uint32* src, int sw, int sh, uint32* dst, int dw, int dh) = 0; };`
   Versie 1 levert alleen `NearestScaler`. Nieuwe filters zijn dan een extra klasse, geen ombouw.
3. **Uitvoerbuffer los van venstergrootte:** de backend vraagt de doelgrootte op (venster of fullscreen) en laat de scaler daarin tekenen.
4. **Keuze in settings:** veld `scaler=nearest` in de settingsfile (en later een toets om te wisselen, zoals F10/F11 voor sets).
5. **Testbaarheid:** scaler-uitvoer op vaste testbeelden vergelijken (golden images), net als de framebuffer-dumps.
6. **Geen OpenGL- of Vulkan-code in versie 1.**

## Vergelijking met echte MoG-graphics (2026-10-14)

Beelden: `beelden/filtervergelijking-original.png` en `beelden/filtervergelijking-hinox.png` (zelfde kamer, map0509, schaal als fullscreen 1920x1080).

Bevinding: de set `original` is eigenlijk 320x200 pixel art, opgeslagen als 640x400 met verdubbelde pixels (meer dan 99% van de pixels is dubbel). De sets naramura, hinox, boltian en alternate zijn echte 640x400 (54 tot 77% dubbel).

Gevolg voor de `Scaler`: bij de set `original` moet een filter (xBR, HQx, CRT) op het halve beeld (320x200) werken, anders ziet het filter alleen 2x2-blokken en doet het vrijwel niets. Bij de andere sets op 640x400. De scaler moet dus per set een "bronfactor" krijgen (1 of 2), bijvoorbeeld via een klein bestand in de setmap of automatische detectie bij het laden.

Indruk:
- Sharp bilinear: beste algemene keuze, trouw aan het origineel, geen ongelijke pixels bij elke schermgrootte.
- xBRZ / xBR: gladste contouren, vooral mooi op de grove `original`-set; maakt soms vormen "plastic". xBRZ zelf is GPL v3 (bevestigd in de broncode), dus voor MoG (GPL v2) eerder Hyllian's xBR (MIT) nemen.
- HQx: lijkt op xBR, iets rommeliger randen.
- CRT: sfeer, maar donkerder; de eenvoudige versie in de vergelijking is een benadering, echte CRT-shaders zijn beter.

## Status

Geïmplementeerd in versie 2 (0.63.2), zie `07-voortgang.md`. Afwijking van het plan: na xBR wordt bilineair geresampled (niet sharp bilinear), omdat xBR al op bijna de eindgrootte uitkomt; bronfactor wordt automatisch per set bepaald.

## Besluit (2026-10-14)

Akakor kiest **xBR** als filter voor versie 2.

- Implementatie: Hyllian's xBR-lv2 (bron: github.com/libretro/glsl-shaders, `xbr/shaders/xbr-lv2.glsl`, MIT-licentie bevestigd), omgezet naar C++ voor de CPU. Copyright- en licentietekst meenemen in `src/third_party/`.
- Geen xBRZ (GPL v3, botst met GPL v2 van MoG).
- Werkwijze: xBR op bronresolutie (320x200 bij `original`, 640x400 bij de andere sets), daarna sharp bilinear naar venster- of schermgrootte.
- Instelling: `scaler=xbr` (standaard), `scaler=sharp` en `scaler=nearest` als alternatief; wisselen met een toets.
- Testen: golden images per set en schaalfactor; prestatie meten in de VirtualBox-VM (doel: 25 fps zonder haperen in fullscreen 1920x1080).
