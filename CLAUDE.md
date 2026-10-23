# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projektziel

Eine ausgemusterte **Bruker-BSMS/2-Tastatur** (Bedieneinheit der BSMS/2-Steuerung eines Bruker-NMR/MRT) soll als **USB-HID oder MIDI-Controller** am PC weiterverwendet werden. Dafür reverse-engineeren wir die Hardware mit einem **Arduino Mega 2560**.

Prioritäten:
1. Großes **Keyboard-Display-Board**: Tasten, LEDs, Display.
2. Großer **Drehregler** (optischer Encoder).
3. Die kleine **Controller-Platine** unten im Gehäuse fliegt voraussichtlich raus. Sie wird nur noch als Referenz untersucht.

### Vorgehen (Phase 1: Tasten und LEDs)

- Das Display-Board bleibt dauerhaft vom Controller-Board abgesteckt. Es wird **nur vom Arduino Mega über USB** versorgt, nicht über die originale 10-V-Versorgung.
- **Schritt 1, stromlos:** mit dem Multimeter GND, VCC, LED-Versorgung (UDN2982A Pin 9) und ULN2804A Pin 10 (COM) identifizieren und in `PINOUT.md` eintragen.
- **Schritt 2, Versorgung anschließen:** Versorgungsadern an 5V/GND des Mega, nicht an I/O-Pins. Die UDN2982A arbeiten auch mit 5 V als Versorgung; die LEDs leuchten dann evtl. schwächer als mit original 12 V.
- **Einschaltregeln:**
  - /OE der 74HC595 (Blanking) per 10k-Pull-up auf 5 V legen bzw. vom Sketch sofort HIGH setzen. Sonst leuchten nach dem Einschalten zufällige LEDs.
  - Sketches starten mit allen Pins auf INPUT.
- **Strombudget:** USB liefert ca. 500 mA. Deshalb immer nur wenige LEDs gleichzeitig ansteuern, nie den kompletten Displaytest (Original: 1,2 A).

**Benennung:** Die Kabeladern heißen P01–P64 (siehe `PINOUT.md`). Für die beiden Rail-Gruppen nutzen wir die Board-Beschriftung **JU1** (P01/03/05/07/09 = GND) und **JU2** (P61–P64 = GND des ULN2804A, vermutlich separater Leistungs-GND) sowie **P60** (= DLED, Daten der 595-Kette, bestätigt 2026-10-23; früher irrtümlich als +5 V Logik geführt) und die Messpunkte **TP1** (P52), **TP2** (P17/19/21/23) und **TP3** (P11/13/15).

**Achtung:** `conn_scan` darf in der versorgten Verdrahtung **nie** laufen. Der Sketch zieht reihum Pins auf LOW, und an 5V hängende Adern würden dabei kurzgeschlossen. Die Arduino-Pins sind kein Ersatz für die Versorgung: max. 20–40 mA pro Pin, die HDSP-2112 an TP3 brauchen deutlich mehr.

**Sicherheitsregel für Sketches:** Solange das Board unversorgt ist, nie einen Pin HIGH treiben, sonst wird das Board über die Schutzdioden der ICs rückgespeist. Nur OUTPUT LOW oder INPUT/INPUT_PULLUP.

**Arbeitsregel:** Alle Erkenntnisse (Messwerte, Pinbelegung, Entscheidungen) sofort festhalten: Pinbelegung in `PINOUT.md`, alles andere in dieser Datei. Die Projektsprache ist Deutsch.

## Verzeichnis

- `Manuals/`: Original-PDFs von Bruker/Spectrospin. Text extrahieren mit `pdftotext -layout Manuals/<datei>.pdf -`.
  - `z31185.pdf`: BSMS User Manual. Tastenfunktionen, Menümodus, Varianten BOSS und HR-20.
  - `z31188.pdf`: BSMS Mainframe Technical Manual (1997).
  - `z31476.pdf`: BSMS/2 Mainframe Technical Manual (1999). Kap. 5 „BSMS Keyboard KBC“ enthält das Blockschaltbild der Tastatur.
  - `z31186.pdf`: BSMS Shim Manual (SCB). Nennt „BOSS Keyboard Z012706“ als Voraussetzung für BOSS2.
  - `z31314.pdf`: B-HPCU High Power Control Unit User Manual (1997). Kap. 2.3.3 „Keyboard I/O“, Tab. 4.4 Steckerbelegung der Tastatur, Kap. 10 Bedienung der HPCU-Tastatur. Das ist **dieselbe Tastaturfamilie** (Z032706).
  - Quelle aller PDFs: Bruker Advanced Service Handbook (BASH), Spiegel an der UIC: https://2210pc.chem.uic.edu/nmr/downloads/BASHCD10/contents.html (direkte Links `.../BASHCD10/pdf/z<nr>.pdf`).
- `Fotos/`: Fotos der geöffneten Tastatur. Die Bilder sind groß (bis 3072×4096); das Read-Tool skaliert sie herunter, kleine Beschriftungen ggf. gezielt nachfragen.
- `Fotos_web/`: verkleinerte Kopien (max. 1600 px) für das Git-Repo. `Fotos/` (Originale) steht in `.gitignore`.
- `README.md`: Projektbeschreibung für GitHub mit Bildern, kompakter Pinbelegung und Ausblick. Bei neuen Erkenntnissen mitpflegen.
- `docs/img/`: Bilder für die README (u. a. das Originallayout aus z31185, Figure 2).
- `PINOUT.md`: Messtabelle und Belegung der 64 Flachbandkabel-Adern sowie des Encoders.
- `tools/serial_log.py`: serieller Mitschnitt mit Zeitstempel (Windows-Python), Aufruf: `python.exe tools\serial_log.py COM8 <logdatei> [sekunden] [befehl]`. Das Öffnen des Ports setzt den Mega zurück.
- `tools/matrix_finder.py`: interaktives Programm, das **der Nutzer selbst in PowerShell startet**: `& "$env:LOCALAPPDATA\Programs\Python\Python313\python.exe" tools\matrix_finder.py COM8`. Es fragt jede der 35 Tasten der Reihe nach ab und erkennt sie automatisch per Scan. Das Ergebnis landet in `logs/matrix_finder_<zeit>.json`.
- `tools/keyboard_gui.py`: grafisches Tasten-Feedback (tkinter, Layout wie Display-Board/Figure 16). Braucht den Sketch `keys`. Zeigt den echten Zustand aller LEDs (Tasten, Bargraphs, LED1–19, LED20–24). Ein Klick auf eine Router-LED oder ein Bar-Segment schaltet es um (`R<n>=`, `B<i>=`), ein Rechtsklick auf ein Bar-Segment setzt den Pegel bis dorthin (`C<k>`). Das Router-Display ist nach der Frontblende gezeichnet, mit Beschriftung, Farbe und Pfeilform. Tasten mit Sonderfunktion haben ein gelbes Kürzel, eine Legende steht rechts. Ein Mausklick auf eine Taste wirkt wie ein Tastendruck (`K<n>`). Das Display spiegelt den echten Text beider HDSP-2112. Über das Eingabefeld rechts lässt sich mit Enter eigener Text auf die Displays schicken. Start durch den Nutzer: `& "$env:LOCALAPPDATA\Programs\Python\Python313\python.exe" tools\keyboard_gui.py COM8`.
- `tools/led_finder.py`: interaktive LED-Suche. Lässt jede Kandidaten-Ader blinken und fragt den Nutzer, was blinkt. Braucht `led_probe` und TP2 (P17) an 5V.
- `tools/pin_tracer.py`: interaktive Leitungsverfolgung. Ein Jumperkabel an **D13** dient als Messspitze; der Nutzer hält es an einen IC-Pin, das Programm meldet, welche Kabelader dort ankommt (direkt oder invertiert). Braucht den Sketch `pin_tracer`. Vorgabe-Messpunkte: IC1 und IC3 Pin 1–8; eigene Namen (z. B. `DIS1.13`) sind möglich. TP2 dabei **nicht** an VIN.
- `tools/router_finder.py`: interaktiver Test der 595-Kette (braucht `led_probe`, TP2 an VIN). Hält P26 LOW, löscht die Kette (P55) und schiebt dann je Enter einen Takt (P56) nach, mit Übernahme (P57). Der Nutzer beschreibt, was sich ändert.
- `tools/ser_finder.py`: sucht die Datenleitung der 595-Kette per Halbierung (Gruppe von Adern LOW, ein Takt, der Nutzer meldet, ob das erste Bargraph-Segment leuchtet). Braucht `led_probe`, TP2 an VIN.
- `tools/led_test.py`: kompletter LED-Test mit Sketch `led_test` (TP2 an VIN). Schritt 1–40: 595-Kette mit k Einsen gefüllt (es kommt jeweils eine LED dazu). Schritt 41–82: jede Matrix-LED einzeln. Das Programm zeigt an, welche LED erwartet wird. Tasten: Leertaste = weiter, b = zurück, a = Automatik, f = fehlt vermerken.
- `tools/router_brute.py`: Brute-Force-Suche nach einem Weg, Router-LEDs einzeln zu schalten (braucht `router_brute`, TP2 an VIN). Modus `wackel [k …]` (Vorgabe 40 und 0) läuft automatisch: Kette mit k Einsen füllen, dann flackert jede Ader 2 s lang mit 4 Hz. Der Nutzer drückt nur eine Taste, wenn oben etwas flackert. Vermerkt werden die aktuelle und die vorige Ader, am Ende folgt ein Einzel-Nachtest der Verdächtigen. Zur Probe flackert am Anfang P26. Modus `zufall [ms]`: Zufallsfolgen auf P26/P55/P56/P57; der Nutzer drückt eine Taste, sobald ein Muster mit Lücke leuchtet. Modus `einzeln [ms]`: jede der 40 Kettenpositionen allein über DLED (P60 an D14), mit Anzeige der erwarteten LED.
- `logs/`: Mitschnitte der Messläufe, benannt nach Datum und Sketch.
- `arduino/`: Arduino-Sketches, ein Ordner pro Sketch.
  - `pin_tracer/`: zieht jede Ader 5× kurz auf ihren Gegenpegel und liest dabei die Messspitze D13 (immer INPUT_PULLUP, treibt nie). Antwort `T <ruhe> P<nr>+ … E` (+ = folgt, - = invertiert, ! = Konflikt).
  - `display_test/`: erster Schreibtest der HDSP-2112. Beim Start: Reset, Helligkeit 40 %, Text „DIS1 ok“/„DIS2 ok“. Befehle `1<text>`/`2<text>` (bis 8 Zeichen), `H<0-7>` Helligkeit (0 = 100 %), `X` Selbsttest, `R` Reset. TP2 wird nicht gebraucht.
  - `led_test/`: steuert die 595-Kette (P26/P55/P56/P57) und die LED-Matrix direkt an; Befehle n/b/a/0/m, Ausgabe `STEP i`.
  - `router_brute/`: Befehle `K<k>` (Kette mit k Einsen, /OE LOW), `P<pos>` (nur Position pos an, über DLED = P60 an D14), `B<nr>[,<ms>[,<n>]]` (Ader n× LOW/HIGH wackeln, Vorgabe 700 ms und 3×, mit Konfliktschutz), `Z<ms>` (Zufallsfolgen der vier Kettensignale), `R`. Test ohne Netzteil 2026-10-23: P24 und P32 lassen sich ohne Konflikt LOW und HIGH treiben.
  - `led_probe/`: treibt auf Befehl einzelne Adern statisch LOW/HIGH (`L<nr>`, `H<nr>`, `R`), mit Rücklese-Schutz gegen Konflikte. Wird von `tools/led_finder.py` gesteuert. Vorprüfung 2026-10-09: Alle 31 im Ruhezustand HIGH liegenden Adern lassen sich ohne Konflikt auf LOW ziehen, P02/P52 ohne Konflikt auf HIGH. Es sind also alles Eingänge mit Pull-up bzw. Pull-down.
  - `keys/`: **Demo-Firmware für das ganze Board** (seit 2026-10-13 inkl. Displays). Liest die Tastenmatrix, multiplext Tasten- und Status-LEDs und steuert die 595-Kette über DLED (P60 an D14), alle 40 Positionen einzeln (seit 2026-10-23). Tastenfunktionen: ONAXIS/2nd = Pegel der Bars +1/−1 (0–20, erst FWD, dann REFL; gehalten mit Wiederholung: 500 ms, dann alle 80 ms), STD BY = Pegel 0 und alle Router-LEDs aus, TRANS. P-DOWN = Lauflicht über LED1–19 mit mitlaufenden Bars (ihre eigene LED leuchtet dabei), CHAN. SELECT = Kanal H → X → Y → Z → aus (Pfeil-LEDs LED6/5/14/15), HE LEVEL/leere Taste/LOCK ON-OFF/LOCK DC/LOCK SHIFT = LED20–24 blinkend an/aus, FINE = Display-Helligkeit eine Stufe heller (13 → 20 → 27 → 40 → 53 → 80 → 100 %, dann wieder 13 %; Start mit 100 %; gehalten alle 400 ms weiter; der HDSP-2112 kennt nur diese 8 festen Stufen, 10-%-Schritte gehen nicht), alle anderen schalten ihre eigene LED. Die Displays zeigen den Tastennamen mit AN/AUS bzw. „PEGEL k / 20“ oder „KANAL H“, beim Start „BSMS/2 READY“. Drehregler = Pegel der Bars einstellen; bei laufender SWEEP-Animation steuert er das Männchen (8 Schritte je Pixel, rückwärts gespiegelt, am Rand Umlauf) (Encoder A = D19, B = D21, Versorgung als Notlösung über D20 HIGH; 24 Schritte je Stufe; Meldung `E <zähler>`, die GUI dreht den Knopf mit). DIFF. MODE = Uhr an/aus („09:15:42  13.10.“, Uhrzeit links, Datum rechtsbündig; mit Jahr passt es nicht, weil der Spalt zwischen den Displays nicht breiter ist als zwischen zwei Zeichen; die Zeit kommt vom PC: Die GUI schickt `Z<Ortszeit in s seit 1970>` beim Verbinden und jede Minute). SWEEP = Display-Animation an/aus: Ein Strichmännchen aus selbstdefinierten Zeichen (UDC) läuft pixelweise über die 16 Stellen und verschwindet dabei im Spalt zwischen zwei Zeichen 3 Pixel lang wie hinter einem Pfosten (`pitch` = 5 + Spalt virtuelle Spalten je Zeichen, Spalt vorgegeben 3; SWEEP AMPL. schaltet den Spalt 0–5 Pixel zum Vergleichen durch; die UDC-Paare 0/1 und 2/3 werden doppelt gepuffert, sonst blitzt die verlassene Zelle auf). Danach folgt die Laufschrift „*** BRUKER BSMS/2 ***“, endlos; jede andere Taste stoppt sie. Meldet `D/U SWn`, `L SWn 0|1`, `S c 0|1|2` (2 = blinkt), `B <hex>` (Bar-Segmente, Bit i = Kettenposition 1+i), `R <hex>` (Router-LEDs, Bit i = Kettenposition 21+i), `W <16 Zeichen>` (Display-Text), `H b` (Helligkeit). Befehle u. a. `W<text>` (Text auf die Displays, UTF-8; Ä Ö Ü ä ö ü ß und die Pixel-Emojis ☺ ☹ ♥ ♪ ★ stehen als selbstdefinierte Zeichen in UDC 4–15, viele Emoji-Varianten wie 😀 🙂 ❤ ⭐ werden darauf abgebildet; UDC 0–3 gehören dem Strichmännchen), `H<0-7>`, `K<n>` (Tastendruck nachahmen), `L<n>=`, `S<c>=`, `C<k>` (Pegel), `B<i>=` (einzelnes Bar-Segment 1–20), `R<n>=` (Router-LED n, 0 = TRANS. P-DOWN), `T0|1`, `A0|1`, `Q` (Details im Kopfkommentar). LEDs brauchen TP2 an VIN.
  - `matrix_probe/`: Brute-Force-Suche nach den Tasten-Zeilenleitungen bei versorgtem Board. Wird von `tools/matrix_finder.py` gesteuert. Pro Scan wird jede Ader, die keine Versorgung, kein Sense und keine Direkttaste ist, für 150 µs auf LOW gezogen (P02 auch auf HIGH); dabei werden alle Pegel gelesen. Ein Scan dauert ca. 90 ms. Ohne Tastendruck gab es keine Kopplungen und kein Flackern (Test 2026-10-09).
  - `line_state/`: rein lesend für das **versorgte** Board. Klassifiziert jede Ader (vom Board HIGH/LOW getrieben oder offen) und meldet danach jede Pegeländerung. Treibt nie einen Pin.
  - `conn_scan/`: passiver Durchgangsscanner. Das Board ist dabei unversorgt, nur JU1 hängt an GND. Der Sketch treibt immer nur einen Pin LOW, alle anderen bleiben INPUT_PULLUP, und meldet dann Verbindungen zwischen Adern sowie Änderungen beim Tastendruck.

## Toolchain (WSL → Windows)

Claude Code läuft in WSL. Das Projekt liegt bewusst auf `C:` (`/mnt/c/...`), damit Windows-Tools direkt darauf zugreifen können. WSL2 sieht USB-/COM-Ports nicht ohne `usbipd`. Deshalb werden die Windows-Programme per WSL-Interop aufgerufen:

- arduino-cli (gebündelt mit der Arduino IDE):
  `"/mnt/c/Users/wbd/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"`
  - Kompilieren: `arduino-cli.exe compile --fqbn arduino:avr:mega <sketch-ordner>`
  - Hochladen: `arduino-cli.exe upload -p COMx --fqbn arduino:avr:mega <sketch-ordner>`
  - Ports anzeigen: `arduino-cli.exe board list`
  - Windows-Programme erwarten Windows-Pfade, also mit `wslpath -w` umwandeln.
- Windows-Python (z. B. mit pyserial) zum Mitlesen der seriellen Ausgabe: `/mnt/c/Users/wbd/AppData/Local/Programs/Python/Python313/python.exe`
- Der COM-Port ist nur von einem Programm gleichzeitig nutzbar. Den seriellen Monitor der Arduino IDE also schließen, bevor wir hochladen oder mitlesen.

## Hardware-Fakten (aus Handbuch und Fotos)

- **Controller-Board** (fliegt raus):
  - Mikrocontroller der 80C535-Familie (Infineon), Firmware-EPROMs `KBC0 AA01` / `KBC0 AA02`.
  - Zum BSMS-CPU geht ein 25-poliges Kabel mit RS-485 (TXD±/RXD±) plus differentiellen Leitungen DLED/CLK/STROBE/TPD/TOGGLE.
  - Versorgung VPWR 9–11 V. Das Board erzeugt daraus +5 V und +12 V.
  - Am Board hängen der Buzzer und der Encoder.
  - Verbindung zum Display-Board über Steckerleiste J3 (64-polig, mit Verriegelung).
- **Keyboard-Display-Board:** Siebdruck „BSN-20 Keyboard-Display-Board“, Spectrospin AG, Z5P2844B. Laut Blockschaltbild drei Teilsysteme:
  1. **Router-Display / Status-LEDs** (LED1–19, zwei 10er-Bargraphs HDSP-4830): Kette aus 74HC595 (IC10–IC14) mit High-Side-Treibern UDN2982A (IC5–IC9). Signale DLED (Daten), CLK, STROBE, Blanking.
  2. **Tasten-/LED-Matrix:** 35 Taster SW1–SW35, jeweils mit LED. Gesteuert über LED-/Tasten-Spaltenauswahl, LED-Zeilenauswahl, Tasten-Zeilenauswahl und Tasten-Spalten-Sense (IC1–IC4, u. a. UDN2982A, ULN2804A).
  3. **Alphanumerisches Display:** 2 × **HDSP-2112** (DIS1, DIS2), je 8 Zeichen 5×7, „intelligentes“ Display mit eigenem Zeichensatz und parallelem 8-Bit-Bus.
  - **IC2 = SN74HC05N:** 6 Inverter mit Open-Drain-Ausgängen. Hypothese: treibt die 6 Tasten-Zeilen (Key Row Select). Die Taster sind in 6 Reihen zu je max. 6 Tasten angeordnet: SW1–5, SW6–11, SW12–17, SW18–23, SW24–29, SW30–35.
  - **IC4 = SN74HC245N:** 8-Bit-Bustreiber. Hypothese: puffert den Datenbus D0–D7 der Displays oder liest die Tasten-Spalten zurück.
  - Außerdem auf dem Board: die Signale T_POWER DOWN und CHAN. SELECT sowie eine „Version Display Board“-Kennung.
- **Encoder:** optisch, S1-256-BM6, 256 Striche pro Umdrehung, Quadratur. Pins: 1 GND, 2 NC, 3 Ch A, 4 +5 V, 5 Ch B.
- **Stromaufnahme** der ganzen Tastatur: 240 mA im Standby, 1,2 A beim Displaytest. Zum Leuchten aller LEDs wird später eine externe Versorgung nötig; USB-5 V reichen nicht.
- **Tastenbelegung des Display-Boards** (laut Foto): LIFT ON-OFF, SPIN RATE, SPIN ON-OFF, CHAN. SELECT, TRANS. P-DOWN / FIELD, LOCK PHASE, LOCK POWER, LOCK GAIN, SWEEP, SWEEP AMPL. / HE LEVEL, (leer), LOCK ON-OFF, AUTO LOCK, LOCK DC, LOCK SHIFT / Z0–Z5 / X, Y, X²-Y², XY, X³, Y³ / ONAXIS, AUTO SHIM, DIFF. MODE, STD BY, FINE, 2nd.

## Varianten der Tastatur (Recherche 2026-10-15)

Bruker-Teilenummern der Form **Z0n2706**: Die Ziffer n kennzeichnet die Variante, wie bei den Shim-Karten SCB13R/L/M = Z002709/Z012709/Z022709.

| Teilenummer (Typenschild) | Typ | Gehört zu | Belege |
|---|---|---|---|
| **Z002706** | BSMS-HR20 | BSMS mit HR-20-Shimsystem (20 Shims) | Typenschild beim Nutzer; IET-Teileliste AMX 400: „Remote Status Unit BSMS-HR20-EC00 Z002706“; z31185 Fig. 3 |
| **Z012706** | BSMS KEYBOARD BOSS | BSMS mit BOSS1/BOSS2-Shims, **unser Gerät** | Typenschild; z31186: „BOSS Keyboard Z012706“; eBay/LabX/IET-Angebote |
| **Z032706** | B-HPCU | High Power Control Unit (Festkörper-NMR: Verstärker X/Y/H, 4-Phasen-Modulator, MAS-Rotor) | Typenschild; IET: „Remote Status Unit B-H PCU-EC-00 Z032/06“; z31314 Kap. 10 |

Was die drei laut Handbüchern gemeinsam haben:
- Gleiches Gehäuse und gleiches Tastenraster:
  - oben das Router-Display mit 2 Bargraphs
  - darunter 3 Tasten links und 2 Tasten rechts
  - 2 × 6 Tasten, 5 Status-LEDs, das 2×8-Zeichen-Display
  - 3 × 6 Tasten und der Drehregler
- Die Varianten unterscheiden sich in Frontfolie und Tastenkappen, in der Firmware des Controllers und vermutlich in der Bestückung.
- **Arbeitshypothese:** Die Display-Platine (BSN-20) ist bei allen gleich oder fast gleich. Unser Reverse-Engineering gilt dann für alle drei.

Unterschiede der Beschriftung:
- **HR-20 statt BOSS:**
  - Kein ONAXIS, UNDO SHIM, POS/SEL, AMPL.
  - Dafür N LEVEL, SHIM MATRIX (2nd + SHIM MATRIX = Menü) und andere Shim-Tasten (X, Y, XZ, YZ, Z, Z², …, XYZ, XZ³).
  - Die beiden linken Tasten der zweiten Lock-Reihe sind leer.
- **HPCU:**
  - Router-Display mit ERR, CW, ⊓, HV pro Verstärker X/Y/H und den Bars mit FOR/REF/HIN-LEDs.
  - 4 PH ENABLE, AMPL/PHASE-Tasten für FH/FX, TUNE/GAIN, MAS-Tasten (BEARING, DRIVE, SPIN, INSERT, EJECT …).
  - Status-LEDs MAS STATUS AUTO / MAIN ON / SPIN LOCK sowie FUNCTION 2nd / MENU.
  - **Oben rechts sitzen CHANNEL SELECT und DISP FINE**, also auf den Positionen unserer Direkttasten CHAN. SELECT/TRANS. P-DOWN.

Erkenntnisse aus dem HPCU-Handbuch (z31314), die zu unseren Messungen passen:
- „The HPCU keyboard gets a **bitstream with 40 bits** … Each bit takes a single clock pulse, and all 40 bits require **one strobe pulse**“ → genau unsere **595-Kette mit 40 Positionen**. Die Signale heißen DLED, DCLK, DSTRB. Bei der BSMS heißen sie DLED, CLK, STROBE, laut z31476 Fig. 17.
- „Two of the keyboard switches are handled directly (not via SIO)“, bei HPCU TOGGLE und FINE, bei BSMS TOGGLE und TPD → unsere beiden **Direkttasten P59/P58**. TPD = T_POWER DOWN = TRANS. P-DOWN, TOGGLE = CHAN. SELECT.
- Alle Signale zwischen Gerät und Tastatur laufen über RS-485 (75174/75175), seriell mit 9600 Baud 8N1. Die Versorgung beträgt ca. 10,9 V unstabilisiert; die Tastatur hat einen eigenen Regler.
- Belegung des 25-poligen Mini-D-Steckers (HPCU, Tab. 4.4):
  - AGND 1, 8, 9, 20, 21
  - 10,9 V auf 10, 11, 22, 23
  - DLED ± 3/15, DCLK ± 4/16, DSTRB ± 5/17
  - TOGGLE ± 6/18, FINE ± 7/19
  - KBC_TXD ± 12/24, KBC_RXD ± 13/25
  - Bei der BSMS gehört dazu das Kabel 22887, für das Router-Display das Kabel 22886.
- Status-LED-Zeile P02 (LED20–24): Bei der BSMS heißen diese LEDs laut Frontfolie SAMPLE UP / DOWN / MISSING und FUNCTION 2ND / MENU. Daraus ergibt sich die vermutliche Zuordnung LED20 = UP, LED21 = DOWN (grün), LED22 = MISSING, LED23 = 2ND, LED24 = MENU (laut z31185 „red Function Menu LED“).
- **Offene Frage:** Laut Handbuch kommen die Router-Daten (DLED) seriell vom Gerät. Bei unserem Board hängt SER von IC10 aber fest an +5 V (TP3). Möglich ist eine Bestückungsvariante, also eine Brücke bzw. ein 0-Ω-Widerstand nahe IC10 oder auf dem Controller-Board. **Board um IC10 Pin 14 auf eine Lötbrücke oder einen Jumper absuchen.** Dann wäre die Kette ohne Leiterbahn-Umbau frei beschreibbar.

**Weitere Quellen im Netz (Recherche 2026-10-15):**
- Handbücher (BASH-Spiegel UIC, siehe oben): außerdem Router (z31215) und Power Router (z31315, mit Schaltplänen; erzeugt die FWD/REFL-Spannungen für die HPCU-Bargraphs).
- HTML-Fassungen der BSMS-Handbücher und Beginners Guide: http://triton.iqfr.csic.es/guide/man/bsms/chap1.1.htm, http://triton.iqfr.csic.es/guide/man/beginners/chap6-2.htm
- Gebrauchtangebote mit Fotos: eBay (Z012706, auch „/1005“, mit Kabel 22887), LabX, IET (https://www.ietltd.com//NMR/BSMS-Boss-Keyboard/).
- **Keine** Schaltpläne der Tastatur und **keine** Bastelprojekte (Umbau, Arduino, MIDI) gefunden. Das Projekt hier ist vermutlich das erste dieser Art.

## Nützliche IC-Pinouts zum Durchklingeln

| IC | Pins |
|---|---|
| 74HC595 | 8 GND, 16 VCC, 14 SER (Daten), 11 SRCLK (Takt), 12 RCLK (Latch/Strobe), 13 /OE (Blanking), 10 /SRCLR, 9 QH' (Kaskade) |
| UDN2982A | 1–8 Eingänge, 9 VS (LED-Versorgung!), 10 GND, 11–18 Ausgänge |
| ULN2804A | 1–8 Eingänge, 9 GND, 10 COM (Freilaufdioden-Versorgung), 11–18 Ausgänge |
| SN74HC05 (IC2) | 1A→2Y, 3A→4Y, 5A→6Y, 9A→8Y, 11A→10Y, 13A→12Y (Eingang→Open-Drain-Ausgang), 7 GND, 14 VCC |
| SN74HC245 (IC4) | 1 DIR, 2–9 A1–A8, 10 GND, 11–18 B8–B1, 19 /OE, 20 VCC |
| HDSP-2112 (DIS1/2) | 1 /RST, 2 /FL, 3–6 A0–A3, 7–9 NC, 10 A4, 11 /CLS, 12 CLK, 13 /WR, 14 VDD, 15 GND (Versorgung), 16 GND (Logik), 17 /CE, 18 /RD, 19 D0, 20 D1, 21–22 NC, 23–28 D2–D7 |

HDSP-2112-Ansteuerung (Quelle: arnowelzel.de/en/hdsp-211x, Datenblatt Broadcom AV02-0629EN):
- Schreibzyklus: /CE low, /FL, A0–A4 und D0–D7 anlegen, dann /WR kurz low. /RD bleibt high.
- Mit /FL=1 wählen A4/A3 den Bereich: 11 = Zeichen-RAM (A0–A2 = Position, D0–D7 = ASCII-Code), 10 = Control Word (D0–D2 Helligkeit, D6 Selbsttest, D7 Clear), 01 = UDC-RAM, 00 = UDC-Adresse.
- Mit /FL=0 wird der Flash-RAM beschrieben (Blinken pro Zeichen).
- Für zwei Displays am selben Bus ist ein getrenntes /CE pro Display zu erwarten.

**Schaltpläne:** Die Handbücher enthalten nur Schaltpläne der Netzteile (z31188 Kap. 7, z31476 Kap. 8), **keine** der Tastatur. Die Belegung muss gemessen werden.

## Erkenntnisse aus Messläufen

- **2026-10-09, `conn_scan` bei unversorgtem Board** (Log: `logs/2026-10-09_conn_scan_unversorgt.txt`). Der Mega hängt an COM8.
  - **P52 (TP1) liest LOW ohne Treiben**, ist also direkt oder niederohmig mit JU1/GND verbunden.
  - Sonst entsteht ein dichtes Geflecht aus Scheinverbindungen, teils flackernd, z. B. `P04 -> P33` und `P14 -> P46`. Die Ursache sind Schutzdioden der unversorgten ICs, die schwebende VCC-Schiene, Darlington-Basis-Strecken der ULN2804A und LEDs, die bei bis zu 5 V über die Pull-ups schon leiten.
  - **Fazit:** Ein passiver Scan am unversorgten Board ist für die Tastenzuordnung nicht brauchbar. Nächster Weg: erst alle Versorgungsschienen klären, dann das Board über P60/JU1 versorgen und erneut scannen.
  - Auffällig, aber noch ungeklärt: P02–P16 (gerade, 8 Adern) sind untereinander sowie mit P11 (TP3) und P61 (JU2) beidseitig verbunden. Das könnten 8 Matrix-Leitungen über LEDs oder Treiber sein.
- **2026-10-09, `line_state` bei versorgtem Board** (P60 + TP3 an 5V, JU1 + JU2 an GND, TP2 offen). Logs: `logs/2026-10-09_line_state_ruhe.txt` und `..._tasten.txt`.
  - Ruhezustand: P02–P14 (gerade) und P52 sind LOW, fast alle anderen Adern HIGH, auch ohne Arduino-Pull-ups. Das Board hat also eigene Pull-ups. P46–P48 liegen bei 3,4–3,7 V.
  - Beim Durchdrücken aller 35 Tasten reagierten nur **zwei Adern: P59, danach P58** (Abstand 3 s, also zwei benachbarte Tasten).
  - Hypothese: Das sind die Tasten **CHAN. SELECT (P59)** und **TRANS. P-DOWN (P58)**, laut Blockschaltbild direkt verdrahtet. Alle anderen Tasten liegen in der Matrix und brauchen eine aktive Ansteuerung der Zeilen/Spalten. **Bestätigt** (`..._verifikation.txt`): 2× CHAN. SELECT → P59, 3× TRANS. P-DOWN → P58. Beide sind direkt verdrahtet, HIGH in Ruhe, LOW gedrückt, und entprellt sauber.

## Router-LEDs einzeln ansteuern? (Analyse 2026-10-23)

Anlass: Der Nutzer und ein Kollege sind sicher, dass im Originalbetrieb einzelne Router-LEDs separat geleuchtet haben.

- **Mit SER = fest 1 geht das grundsätzlich nicht, auch nicht per Software-Trick.** Alle fünf 595 teilen sich SRCLK, RCLK, /OE und /SRCLR. Erreichbar sind nur die Muster „erste k an“. Auch Multiplexen bzw. PWM hilft nicht: Die Helligkeit kann entlang der Kette nur abnehmen. Wenn das Original Einzel-LEDs konnte, muss DLED also irgendwie an IC10.14 ankommen.
- **Das Design spricht für den Kollegen.**
  - Laut z31185 zeigen H/X/Y/Z den gewählten Kanal, MIS einen Mismatch usw.
  - Laut z31314 gibt es einen echten 40-Bit-Datenstrom.
  - Mit „erste k“ wäre LED17 (Position 40) nur zusammen mit allen 39 anderen an, das ergibt keinen Sinn.
  - Einschränkung: Laut z31185 gilt „This display is not supported from all systems“. Möglich ist also eine Bestückungsvariante ohne Router-Daten.
- **Was wir haben:**
  - Das Blockschaltbild (z31476 Fig. 17) nennt vier Router-Signale: DLED, CLK, STROBE, Blanking.
  - Gefunden sind CLK = P56, STROBE = P57 und Blanking = P26, dazu /SRCLR = P55. **Nur DLED hat noch keine Ader.**
- **Verdächtig: P24 und P32.** Beide werden vom Board auf HIGH gezogen (line_state: „getrieben HIGH“), ein Ziel wurde nie gefunden. Damit sind sie Kandidaten für DLED (oder für die „Version Display Board“-Kennung).
- **Fotos:** An IC10 Pin 14 ist weder oben noch unten eine Leiterbahn zu sehen. Die Platine ist mehrlagig (Aufdruck „Layer 1“). Die Verbindung SER ↔ +5 V läuft also in einer Innenlage, eine Brücke wäre von außen nicht sichtbar.
- **Beweislage für SER = TP3:**
  - Sie beruht nur auf dem Durchgangspiepser (2026-10-09 und 2026-10-12). In derselben Sitzung vom 09.10. war „IC10.10 → 5V“ nachweislich falsch, richtig ist P55.
  - Der Tracer-Lauf `pin_tracer_2026-10-12_081754.json` las an „IC10.14“ LOW. Das passt nicht zu fest 5 V. Wahrscheinlich war die Spitze in diesem Lauf verrutscht: „IC4.11/IC4.12“ ergaben genau die Signale von IC10.11/IC10.12 (P56/P57), und „IC10.14“ LOW passt zu IC10.13 (/OE = P26, als Basis LOW gehalten).
- **Plan:**
  - **M1** (stromlos, Ohm-Bereich, nicht Piepser): Widerstand von IC10.14 gegen P11, P13, P15, P60, P24 und P32. Zum Vergleich P11 ↔ P13 und P11 ↔ P15.
  - **M2:** mit `pin_tracer` ermitteln, wo P24 und P32 auf dem Board ankommen (RN8/RN9, R1–R11, R4, freie Pads).
  - **Ergebnis M1 (2026-10-23, Ohm-Bereich, stromlos):** IC10.14 ↔ P60 0,4 Ω, ↔ P11/P13/P15 2 Ω (P11/13/15 sind auf der Platine ein Netz), ↔ P24 und ↔ P32 kein Durchgang. **SER = +5 V ist damit sicher**, und zwar direkt an der 5-V-Fläche (die Werte liegen im Bereich des Messleitungswiderstands). P24/P32 führen nicht zu SER. Im Originalbetrieb kann diese Platine also keine einzelnen Router-LEDs gezeigt haben; der Kollege kennt das vermutlich von einer anderen Variante (z. B. HR-20 bzw. HPCU) oder Bestückung.
  - Der Nutzer bleibt dabei: Bei **diesem** Gerät waren die Router-LEDs im Originalbetrieb definitiv einzeln ansteuerbar. Wenn das stimmt, muss es einen Mechanismus außerhalb unseres Modells geben. Er müsste über eine der 64 Adern laufen (der Controller hat keinen anderen Zugang). Test dafür: `tools/router_brute.py` (Wackeltest aller Adern, Zufallsfolgen der Kettensignale), später ggf. mit Webcam.
  - Erster `wackel 40` am 2026-10-23 **ohne Netzteil** (P17 an VIN): Alle Router-LEDs leuchteten deutlich, LED20–24 blinkten bei P02 nicht. Erklärung: Ohne Netzteil liegt VIN über die Rückwärtsdiode des Spannungsreglers bei ca. 4,3 V. Für Router-LEDs (UDN2982A → LED → GND) reicht das, für Matrix-LEDs (UDN2982A → LED → ULN2804A) nicht. Die Folgerung vom 2026-10-09 „VS = 5 V zu wenig“ gilt also nur für die Matrix. **Strombudget:** 40 LEDs aus USB über den Regler-Rückweg vermeiden; das Tool fragt jetzt vorher nach dem Netzteil.
  - **Wackeltest 2026-10-23 mit Netzteil** (`logs/router_brute_wackel_2026-10-23_144716.json`, k = 40 und 0): **kein Treffer**. Es blinkten immer nur ganze Matrix-Zeilen, oben nie einzelne LEDs. Nicht getestet wurden die Adern an Versorgungen: P60, P11/13/15, P17/19/21/23, JU1, JU2.
  - Präzisierung des Nutzers: Im Original gingen **LED1–19 einzeln**, die Bargraphs LED25/26 nur als Pegel (von links gefüllt). Bei Berührung der IC-Pins oben mit dem Finger leuchten einzelne LEDs (bei /OE HIGH schweben die UDN-Eingänge, das ist also kein Widerspruch).
  - IC-Anordnung oben (Foto `IMG20261009132234`): 595-Reihe IC14, IC11, IC10, IC12, IC13 und UDN-Reihe IC9, IC6, IC5, IC7, IC8 (jeweils in Leserichtung). Alle 595 sind SN74HC595N, alle Treiber UDN2982A. Der Kettenanfang IC10 sitzt in der Mitte.
  - **Neue Hypothese (2026-10-23): P60 ist gar nicht die Versorgung, sondern DLED (= SER von IC10).** Gründe:
    - P55–P60 sind genau der Signalsatz des 25-poligen Kabels: P55 /SRCLR, P56 CLK, P57 STROBE, P58 TPD, P59 TOGGLE, also fehlt nur **DLED → P60**.
    - Alle Versorgungen liegen als Gruppen von 3–5 Adern in der ungeraden Reihe (JU1, TP3, TP2, JU2). Eine einzelne Ader als Logik-Versorgung passt nicht ins Muster.
    - Laut M1 hat IC10.14 0,4 Ω zu P60, aber 2 Ω zu P11. Die 2 Ω laufen vermutlich über die 5-V-Leiste des Mega, an der P60 und P11 beide hängen. Am 2026-10-09 hatte TP3 stromlos keinen Durchgang zu P60.
    - „P60 = IC10.16 (VCC)“ stammt aus der Piepser-Sitzung vom 2026-10-09, in der auch „IC10.10 → 5V“ falsch war (richtig: P55).
    - Alle bisherigen SER-Suchen haben P60 nie bewegt, weil P60 immer fest an 5 V hing. Damit war SER immer 1, und die Kette ließ sich nur mit Einsen füllen.
    - Vermutung dazu: Die 595 bekommen ihr VCC von TP3; über P60 und die Schutzdiode an SER wären sie notfalls mitversorgt.
  - **Prüfung M3** (stromlos, USB ab, P60 vom 5V des Mega abziehen, Ohm-Bereich): IC10.16 ↔ P11 (bzw. TP3), IC10.16 ↔ P60, IC10.14 ↔ P60. Erwartet bei richtiger Hypothese: 16↔TP3 ≈ 0 Ω, 16↔P60 offen, 14↔P60 ≈ 0 Ω. Erst danach P60 an einen I/O-Pin (z. B. wieder D14) legen und als DLED treiben.
  - **M3 bestätigt die Hypothese (2026-10-23, stromlos, P60 vom Mega abgezogen):** IC10.16 ↔ TP3 ≈ 0 Ω, IC10.16 ↔ P60 offen, IC10.14 ↔ P60 ≈ 0 Ω. **P60 = DLED = SER von IC10.** Die 595 werden aus TP3 versorgt. Der Fehler stammte aus der Piepser-Sitzung vom 2026-10-09. Weil P60 seitdem an 5 V hing, war SER immer 1.
  - Neue Verdrahtung: **P60 → D14** (statt 5V). `router_brute` schiebt jetzt echte Daten (`P<pos>` = nur diese Position an), Test mit `router_brute.py COM8 einzeln`. Sicherheitsregel: D14 nie HIGH treiben, solange das Board unversorgt ist (Rückspeisung über SER). Die Sketches lassen D14 bis zum ersten Kettenbefehl auf INPUT.
  - **Durchbruch 2026-10-23** (`logs/router_brute_einzeln_2026-10-23_155044.json`): Alle 40 Kettenpositionen leuchten einzeln, genau wie erwartet. **Router-LEDs, Bars und die LED in TRANS. P-DOWN sind damit frei schaltbar.** Der Nutzer: „Wir haben's!“
  - Während der Messungen ist der Laptop abgestürzt. Das Netzteil hing dabei **nicht** am Mega. Das 9-V-Netzteil ist defekt, als Ersatz ist ein 12-V-Netzteil da. Die Ursache ist ungeklärt; der Mega meldet sich danach normal an COM8.
  - Bestätigt sich SER = +5 V: IC10 sockeln, Pin 14 herausbiegen und per Draht auf P24 oder P32 legen (sofern diese frei sind). Das ist reversibel, und die Ader kommt am Mega schon an (D45/D53).

## Stand und nächste Schritte (aktualisiert 2026-10-12)

- **Versorgung komplett geklärt** (siehe `PINOUT.md`).
- **Alle 35 Tasten funktionieren.** 6×6-Matrix plus 2 Direkttasten, vollständige Tabelle in `PINOUT.md`. Sketch `keys` und GUI `tools/keyboard_gui.py` laufen.
- **Tasten-LEDs funktionieren (2026-10-12)**, mit TP2 an VIN (9-V-Netzteil am Mega). Alle Tasten außer CHAN. SELECT (keine LED) und TRANS. P-DOWN (sitzt in der 595-Kette) laufen über die Matrix und werden vom Sketch `keys` gemultiplext. Router-LEDs: Die Steuerleitungen der 595-Kette sind bekannt; Damals galt: SER fest auf +5 V, die Kette lässt sich nur mit Einsen füllen. **Überholt am 2026-10-23:** SER = P60 (DLED), alle Positionen sind frei schaltbar.
- **LED-Test 2026-10-09 ohne Erfolg** (`logs/led_finder_2026-10-09_164037.json`):
  - Bei TP2 = 5 V leuchtete nichts, weder im Ruhezustand noch beim Blinken einer der 33 Kandidaten-Adern. Es gab keine Konflikte.
  - Wahrscheinliche Ursache: **VS = 5 V ist zu wenig**. In Reihe liegen UDN2982A (ca. 1,7 V), LED (ca. 2 V), ULN2804A (ca. 0,9 V) und der Vorwiderstand.
  - Indiz: Die vorherigen Scans haben P56 (Schiebetakt) und alle anderen Adern hundertfach gepulst. Die 595-Kette dürfte also längst mit Einsen gefüllt und übernommen sein, trotzdem blieben auch die Router-LEDs dunkel. Das ist allerdings nur gültig, wenn SER fest an 5 V liegt (Messung IC10.14 → 5V) und /OE aktiv ist.
  - **Plan nächste Sitzung:**
    - Den Mega mit einem **9-V-Netzteil** versorgen und **TP2 (P17) an VIN** (ca. 8 V) statt an 5V legen. Zuerst das Multimeter in mA in Reihe schalten, um den Strom zu sehen; die Vorwiderstände der LEDs sind unbekannt.
    - Dann `led_finder` erneut laufen lassen.
    - Bleibt es dunkel: Messung A (Spannung an IC1 Pin 9) und Messung B (IC1/IC3 Pins 1–8 per Durchgang zu den Mega-Pins).
- **LED-Test 2026-10-12 mit 9-V-Netzteil, ebenfalls ohne Erfolg** (`logs/led_finder_2026-10-12_073005.json`, `..._073409.json`):
  - P17 (TP2) hing direkt an VIN. An IC1 Pin 9 (VS) lagen **8,6 V**, die LED-Versorgung kommt also an. Ein Ruhestrom war nicht messbar.
  - Trotzdem blinkte bei keiner der 33 Adern etwas.
  - **Folgerung:** Keine einzelne Ader schaltet eine LED. UDN2982A und ULN2804A sind beide aktiv HIGH, eine LED braucht also gleichzeitig einen passenden Zeilen- und Spalteneingang. Oder die Eingänge von IC1/IC3 hängen gar nicht direkt am Kabel.
  - Nächster Schritt: mit `pin_tracer` feststellen, welche Adern an IC1 und IC3 Pin 1–8 ankommen.
- **`pin_tracer` 2026-10-12** (`logs/pin_tracer_2026-10-12_074536.json`, P17 abgezogen):
  - **IC1 (UDN2982A) Pin 1–6:** in Ruhe LOW. Sie gehen auf HIGH, sobald **P26 LOW** wird (invertierte Reaktion), sonst reagiert keine Ader. Deutung: IC1.1–6 hängen an der B-Seite von **IC4 (74HC245)**, und P26 ist dessen /OE. Bei P26 = LOW schaltet der 245 den Bus P46–P51 (A1–A6, Ruhe HIGH) auf IC1 durch. Die LED-Spalten (High-Side) sind also **Bus P46–P51, freigegeben mit P26 = LOW**. Das passt zur Messung IC4.19 (/OE) → P26.
  - IC1 Pin 7/8: keine Ader, Ruhe LOW (vermutlich unbenutzt oder A7/A8 des 245).
  - **IC3 (ULN2804A) Pin 1–6:** Ruhe HIGH (wahrscheinlich nur der Pull-up der Messspitze an einem hochohmigen Eingang), keine Ader gefunden. **IC3.7 folgt P04** (Tasten-Zeile 6). IC3.8: Ruhe LOW, nichts.
  - Hypothese: Die Low-Side (ULN2804A) wird von den **Tasten-Zeilen P04–P14** angesteuert (Zeile HIGH = LED-Zeile aktiv). Das wurde im LED-Test noch nie geprüft, weil die Zeilen dort nicht auf HIGH getrieben wurden.
  - Test: `led_finder.py COM8 L26` hält P26 auf LOW und blinkt zuerst die Zeilen P04–P14 auf HIGH. `led_probe` hat dafür den Befehl `F<nr>` (eine Ader freigeben).
- **LEDs leuchten! 2026-10-12** (`logs/led_finder_2026-10-12_075016.json`, TP2 an VIN, Basis P26 = LOW):
  - **Die Tasten-Zeilen sind gleichzeitig die LED-Zeilen.** Zeile HIGH + P26 LOW → alle LEDs dieser Zeile leuchten (der Bus P46–P51 liegt in Ruhe HIGH, alle 6 Spalten sind also an):
    - P04 → SW30–35, P06 → SW24–29, P08 → SW18–23, P10 → SW12–17, P12 → SW6–11, P14 → SW1–3 (Reihe 1 hat nur 3 Matrix-Tasten).
    - **P02 HIGH** → 7. LED-Zeile: Status-LEDs LED20 (rot), LED21 (grün), LED22 (rot), LED23 (rot), LED24 (rot); LED27 ist nicht bestückt.
  - **Bargraph LED25:** Sobald P26 LOW war, leuchteten die ersten 6 von 10 Segmenten dauerhaft, während der ganzen Messung. Bei **P57 LOW** leuchteten nur noch 2. Ungeklärt: Das kann eine eigene, in Ruhe aktive Zeile sein, oder die 595-Kette (Messung IC10.12 RCLK → P26).
  - **Spalten geklärt** (`..._075909.json`): P46–P51 = Spalte 1–6, gleiche Reihenfolge wie die Tasten. Spalte HIGH = LED an. Die LED-Matrix ist damit vollständig bekannt (Tabelle in `PINOUT.md`).
  - **Test mit Sketch `keys` und GUI (2026-10-12):** Alle Tasten-LEDs funktionieren, außer der LED von **TRANS. P-DOWN** (oben rechts). **CHAN. SELECT hat keine LED.** Die TRANS-P-DOWN-LED liegt nicht in Zeile P14 (dort leuchteten bei allen Spalten HIGH nur SW1–3). Vermutung: Sie hängt an der 595-Kette oder an einer eigenen Ader.
  - Irgendwann während der Tests leuchteten zusätzlich **Router-LEDs LED1–LED18**. Vermutung: Am Ende des Laufs mit Basis L26 wurde P26 freigegeben, und die steigende Flanke an RCLK hat den Inhalt des Schieberegisters übernommen. Der war vorher durch die vielen P56-Pulse (SRCLK) gefüllt worden. Das stützt RCLK = P26 und SRCLK = P56. **Kette angesteuert** (`router_finder`, 2026-10-12): Jeder Takt schiebt eine 1 nach (SER effektiv HIGH). Reihenfolge der LEDs inkl. TRANS. P-DOWN (Takt 27) in `PINOUT.md`; der Bargraph-Teil ist unsicher gezählt. SER-Suche mit `ser_finder` (`logs/ser_finder_2026-10-12_082841.json`): Alle 35 in Ruhe HIGH liegenden Adern gleichzeitig auf LOW → trotzdem wurde eine 1 geschoben, SER ist also keine davon. Lauf `ser_finder.py COM8 hoch` (`..._083012.json`): P02, P04–P14 und P52 auf HIGH → ebenfalls eine 1, also kein Treffer. **SER hängt an keiner am Mega angeschlossenen Ader.** Vermutung: SER schwebt an einer der nicht angeklemmten „Rail“-Adern (P03/05/07/09, P13/15, P19/21/23, P62–64). Darauf deutet auch der Tracer, der an IC10.14 einmal HIGH und einmal LOW las. Nächster Schritt: stromlos durchpiepsen, IC10.14 sowie IC4.8/IC4.9 gegen diese 12 Adern. **Gemessen** (`pin_tracer.py COM8 router`): SRCLK = P56, RCLK = P57, /OE = P26 (gemeinsam mit dem 245!), /SRCLR = P55. IC10 ist der Kettenanfang, sein SER hängt an keiner Kabelader. Vermutet wird die B-Seite des 245, siehe `PINOUT.md`. `pin_tracer` kann dafür Basis-Adern halten (`+L26`).
  - Konsequenz für den Originalbetrieb: Der Controller multiplext Tasten und LEDs über dieselben Zeilen und legt dabei die Spaltenmuster über den 245 (P26 = /OE) auf den Bus.
- **Stand Sitzungsende 2026-10-12:**
  - **Fertig:** alle Tasten, alle LEDs (Tasten-LEDs und LED20–24 per Matrix, Bargraphs/LED1–19/TRANS. P-DOWN per 595-Kette). Der Sketch `keys` ist eine Demo-Firmware mit Tastenfunktionen, und `tools/keyboard_gui.py` zeigt alles an. Der Nutzer ist zufrieden („Sehr schick!“).
  - **Auf dem Mega:** `keys`.
  - **Verdrahtung:** Der Mega hängt am 9-V-Netzteil und an USB (COM8). P17 (TP2) steckt an VIN, D38 ist frei. Ein Jumperkabel an D13 dient als Messspitze für `pin_tracer`. Sonst gilt die Verdrahtung „versorgt“ aus `PINOUT.md`.
  - **Entscheidung offen:** Umbau von IC10 Pin 14 (SER) auf einen Mega-Pin, damit Router-LEDs und Bars frei schaltbar werden. Bisher gehen nur „die ersten k“.
- **Displays, 2026-10-13** (`logs/pin_tracer_2026-10-13_085802.json`, `pin_tracer.py COM8 +L26 dis1`, P17 abgezogen):
  - DIS1 ist vollständig zugeordnet (Tabelle in `PINOUT.md`). Die Reihe oben auf der Rückseite entspricht Pin 15→28, die Reihe unten Pin 14→1.
  - **Alle Display-Signale gehen direkt ans Kabel**, nicht über den 74HC245:
    - Daten D0–D7 = P38, P37, P36, P35, P34, P33, P30, P28.
    - Adressen A0–A4 = P22, P25, P27, P29, P31.
    - Steuerleitungen: /WR = P54, /RD = P53, /CE = P16, /FL = P20, /RST = P39.
  - Die frühere Hypothese „P46–P51 = Display-Datenbus“ ist damit **widerlegt**. P46–P51 sind nur die LED-Spalten.
  - /CLS von DIS1 liegt fest auf LOW, CLK hängt an keiner Ader. DIS1 ist also Slave und bekommt seinen Takt vermutlich vom Master DIS2.
  - **Schreibtest erfolgreich (Sketch `display_test`):** „DIS1 ok“ und „DIS2 ok“ stehen korrekt da, alle Zeichen stimmen.
    - **DIS2 sitzt links, DIS1 rechts**, wie auf der Platine beschriftet. Position 0 ist jeweils das linke Zeichen.
    - Damit sind alle 16 Zeichen von links nach rechts erreichbar: DIS2 Pos. 0–7, danach DIS1 Pos. 0–7.
  - **Displays in `keys` und GUI eingebaut** (2026-10-13), Test per `K`-Befehlen erfolgreich (Ausgabe `W LOCK PHASE    AN`, `W KETTE    1 / 40`).
  - DIS2 (`..._090319.json`): **/CE = P18**, alle anderen Leitungen sind gemeinsam. /CLS liegt auf HIGH, DIS2 ist also der Takt-Master für DIS1. Die freien Adern P24 und P32 haben noch keine Funktion.
- **Stand 2026-10-13:** Die Displays sind fertig. Der Sketch `keys` steuert jetzt das komplette Board: Tasten, alle LEDs, beide Displays mit Text, Umlauten, Pixel-Emojis, Animation und Helligkeit. Der Nutzer ist begeistert („Hammer“).
- **Stand 2026-10-14:** Auch der **Encoder läuft** (A = D19, B = D21, Versorgung als Notlösung über D20 mit 4,6 V gemessen; die Richtung ist korrigiert, 1024 Schritte pro Umdrehung bestätigt). Dazu kamen die Uhr (DIFF. MODE), Umlaute und Pixel-Emojis per UDC sowie das Männchen am Drehrad. **Damit funktioniert die gesamte Bedienhardware.**
- **Offen:**
  1. **USB-HID bzw. MIDI**, das eigentliche Ziel. Optionen A–D siehe „Planung Zielhardware“ unten, der Favorit des Nutzers ist D (RP2350B).
  2. **Strom messen** (Leerlauf, alle LEDs, Displays 100 %, Selbsttest) als Grundlage für die Versorgung.
  3. **Rastermaß des Flachbandkabel-Steckers** prüfen (vermutlich 2,54 mm).
  4. RP2350B-Modul auswählen und bestellen, fliegend testen, dann die Platine in KiCad entwerfen.
  5. Spalt für das Männchen festlegen (Vorgabe zurzeit 3 Pixel, der Nutzer vergleicht mit SWEEP AMPL.).
  6. ~~Umbau IC10 Pin 14 (SER)~~ nicht nötig: SER hängt an P60 (DLED), seit 2026-10-23 sind alle 40 Kettenpositionen frei schaltbar. Für die Zielhardware: DLED ist ein weiterer Ausgang (Option E: 1 GPIO mehr bzw. 1 Bit auf einem 74HCT595).
  7. Optional: Funktion der freien Adern P24/P32 klären, evtl. die „Version Display Board“-Kennung.
  8. ~~GitHub~~ erledigt: Das Projekt kommt als **privates** Repo „bsms-keyboard-boss“ („BSMS Keyboard Boss“) zu GitHub, inkl. Manuals und verkleinerten Fotos. **Gepusht am 2026-10-14: https://github.com/dasaweb/bsms-keyboard-boss** (privat, Branch `main`). `gh` liegt ohne sudo in `~/.local/bin/gh` (v2.102.0) und ist als Konto `dasaweb` per HTTPS angemeldet. Die Git-Zugangsdaten laufen nur repo-lokal über `gh auth git-credential`. Commits in diesem Repo laufen unter `daniel@dasaweb.de` (repo-lokal gesetzt, global bleibt die Fraunhofer-Adresse). Künftig nach Änderungen committen und pushen, wenn der Nutzer es wünscht.
- **Merkpunkt für später (eigene Platine, PCB):** Am Mega werden die 5-V-Anschlüsse knapp. Belegt sind P60, P11 (TP3), der Encoder und ggf. weitere. Auf einer eigenen Platine eine ordentliche 5-V-/GND-Verteilung vorsehen, mit mehreren Abgriffen und ausreichend Strom für Displays und LEDs. Notlösung beim Encoder (2026-10-14): 5 V vom ICSP-Stecker (Pin 2), weil keine 5-V-Buchse mehr frei war. Eine Versorgung über einen I/O-Pin ist zu knapp (Encoder ca. 25–35 mA).
- **Planung Zielhardware und Platine (Diskussion 2026-10-14, noch nichts entschieden):**
  - **Bedarf:** ca. 45 I/O mit 5-V-Pegel.
    - Zeilen 7, Tasten-Spalten 6, LED-Bus 6, 595-Steuerung 5 (P26/P55/P56/P57 + DLED P60), Direkttasten 2, Display 18 (D0–D7, A0–A4, /WR, /RD, 2× /CE, /FL, /RST), Encoder 2.
    - Das Board ist 5-V-Logik. 74HC-Eingänge brauchen bei VCC = 5 V ca. 3,5 V für HIGH, 3,3-V-Controller brauchen also Pegelwandler (z. B. 74HCT245/541). Die Tasten-Spalten haben Pull-ups auf 5 V.
  - **Wünsche des Nutzers:**
    - USB-HID und MIDI; MIDI am liebsten über USB, der 5-polige DIN-Stecker ist optional.
    - USB auch zum Flashen. Optional WLAN/Bluetooth.
    - Shield auf dem Mega mit 2×32-Wannenstecker fürs Flachbandkabel (Rastermaß am Kabel noch prüfen), 4-poliger Encoder-Leiste und Versorgungsbuchse.
  - **Optionen:**
    - **A: Mega + andere Firmware auf seinem USB-Chip ATmega16U2.** Mit HoodLoader2 (NicoHood) kann der 16U2 USB-HID, MIDI und weiterhin Seriell, ohne neue Hardware. mocoLUFA/HIDUINO können nur MIDI. Nachteil: Der 16U2 muss einmalig per ISP neu geflasht werden, mit einem zweiten Arduino als Programmer (vor Umsetzung prüfen).
    - **B: Mega + ESP32-S3 auf dem Shield**, verbunden über UART (TX vom Mega per Spannungsteiler auf 3,3 V).
      - Der ESP32-S3 kann natives USB (HID, MIDI), BLE-HID und BLE-MIDI sowie WLAN. Über WLAN wären auch NTP für die Uhr und eine Weboberfläche möglich.
      - Der Mega bleibt reiner 5-V-I/O-Knecht.
      - Nachteil: zwei Controller, zwei Firmwares, zwei USB-Buchsen.
    - **C: Ein Controller mit nativem USB:**
      - Teensy 4.1 (55 I/O, USB-HID/MIDI fertig, aber 3,3 V → Pegelwandler)
      - Pro Micro 5 V (ATmega32U4) + 3× MCP23017 (5-V-Portexpander)
      - AT90USB1286 (5 V, 46 I/O, natives USB; ehemals Teensy++ 2.0, als Modul kaum noch erhältlich, als Chip TQFP64 für eine erste Platine anspruchsvoll)
      - In allen drei Fällen: neue Platine und neue Firmware.
    - **D: RP2350B auf eigener Platine (Favorit des Nutzers, 2026-10-14, wegen Platz im Gehäuse):**
      - Der normale Pico bzw. Pico 2 hat nur 26 GPIO, das reicht nicht. Der **RP2350B hat 48 GPIO**. Module: Pimoroni PGA2350, WeAct RP2350B Core Board, Solder Party RP2350 Stamp XL (Verfügbarkeit prüfen).
      - Natives USB: HID, MIDI und Seriell gleichzeitig (TinyUSB, Arduino-Core von earlephilhower), Flashen per USB (BOOTSEL). Kein WLAN/BT.
      - **RP2350-Eingänge sind laut Datenblatt 5-V-tolerant**, Tasten-Spalten, Direkttasten und Encoder können also direkt ran (vor dem Layout im Datenblatt verifizieren).
      - Die Ausgänge liefern 3,3 V. Das reicht für die **18 Display-Leitungen** (HDSP-2112 hat TTL-Eingänge, HIGH ab 2,0 V), aber **nicht** für 74HC05/ULN2804 (7 Zeilen), 74HC245 (6 Bus) und 74HC595 + P26 (4).
      - Diese 17 Ausgänge laufen deshalb über **3 × 74HCT541** auf der Platine. Fertige TXS0108E-Pegelwandler-Module sind ungeeignet, weil sie sich mit den Pull-ups/Pull-downs des Boards nicht vertragen.
      - Platine: RP2350B-Modul, 3 × 74HCT541, 2×32-Wannenstecker, Encoder-Leiste, Versorgung (9–12 V → Step-Down 5 V; VS direkt). Die Firmware lässt sich größtenteils aus `keys` übernehmen.
    - **E: Pico 2 / Pico 2 W (RP2350A, 26 GPIO) mit 74HCT595 als Pegelwandler und Port-Erweiterung (neuer Favorit, 2026-10-14):**
      - Direkt angeschlossen bräuchte es **46** Leitungen; davon 19 fürs Display (D0–D7, A0–A4, /WR, /RD, 2× /CE, /FL, /RST).
      - Reduktion:
        - /RD und /FL fest auf HIGH, /RST per RC-Glied (−3).
        - **3 × 74HCT595** auf der Platine (3,3 V rein, 5 V raus) liefern 24 Ausgänge über 3 GPIO: 7 Zeilen, 6 LED-Bus, 4 Kettensignale (P26, P55, P56, P57), A0–A4 und 2× /CE. **Nachtrag 2026-10-23:** Dazu kommt DLED (P60), also 25 Ausgänge. /SRCLR (P55) kann fest auf HIGH, dann reichen wieder 24.
      - Direkt am Pico bleiben: 595-Kette der Platine 3, Display D0–D7 + /WR 9 (3,3 V reichen, TTL-Eingänge), Tasten-Spalten 6, Direkttasten 2, Encoder 2 = **22 von 26 GPIO**, 4 Reserve (z. B. MIDI-DIN).
      - **Es muss ein RP2350 sein (Pico 2), nicht der RP2040 (Pico 1).** Nur der RP2350 hat 5-V-tolerante Eingänge, und Tasten-Spalten und Encoder liefern 5 V. Vor dem Layout im Datenblatt verifizieren. Die 5-V-Toleranz gilt vermutlich nur bei versorgtem IOVDD, also keine 5 V an den Pico legen, solange er stromlos ist.
      - **Pico 2 W** = gleiche GPIO-Zahl plus WLAN/Bluetooth: USB-HID/MIDI, BLE-MIDI/HID, NTP-Uhr, Flashen per USB, alles auf einem kleinen, günstigen Board.
    - **MIDI über DIN** geht mit jeder Option nebenbei: UART-TX (z. B. Serial1 des Mega) + 2 Widerstände + DIN-Buchse. Für den MIDI-Eingang bräuchte es einen Optokoppler.
  - **Weitere Anwendung: Smarthome-Controller (Idee des Nutzers, 2026-10-15)**, z. B. für Home Assistant oder allgemein als MQTT-Schnittstelle:
    - **Eingaben:** Tasten (kurz/lang) schalten Lichter und Szenen. Das Drehrad dimmt bzw. regelt Lautstärke oder Heizung.
    - **Anzeigen:**
      - Tasten-LEDs zeigen den echten Zustand aus HA.
      - Display: Uhrzeit, Temperaturen, Nachrichten, Werte beim Drehen.
      - Bargraphs als Pegel, z. B. PV-Leistung und Verbrauch.
      - Status- und Router-LEDs für Meldungen bzw. Alarme (blinkend).
    - **Umsetzung:** MQTT mit **Home-Assistant-Discovery**. Das Gerät legt seine Entitäten selbst an:
      - Tasten → `event`
      - LEDs → `switch`/`light` (an/aus/blinkt)
      - Display → `text`
      - Drehrad → `number` bzw. relative Ereignisse
      - Kette/Bars → `number` 0–40
    - **Prototyp ohne neue Hardware:** Python-Brücke Seriell ↔ MQTT (paho-mqtt) auf PC/Raspberry Pi. `keys` liefert bereits alle Meldungen und Befehle (`D/U SWn`, `E`, `L`, `S`, `C`, `W`, `Z`). Dafür müssten die Tastenfunktionen per `T0` abschaltbar sein, damit HA allein steuert.
    - **Endausbau:** Pico 2 W per WLAN direkt an MQTT, kein PC nötig (spricht zusätzlich für Option E). Alternative ESP32-S3 + ESPHome (RP2350-Unterstützung von ESPHome noch prüfen).
    - MIDI, HID und MQTT als Modi in **einer** Firmware, umschaltbar z. B. per 2nd + Taste.
  - **Empfehlung Claude:** Erst A ausprobieren (Software, ohne neue Hardware). Das Shield so planen, dass optional ein ESP32-S3-Modul (B) und ein MIDI-DIN-Ausgang bestückt werden können. Vorher ein PC-Programm „seriell → MIDI“ als schneller Test.
  - **Versorgung (vor dem PCB klären):** Strom messen: Leerlauf, alle LEDs an (`A1`), Displays auf 100 %, Selbsttest. Favorit: 9–12-V-Netzteil → Step-Down 5 V (Logik und Displays, am Mega-Regler vorbei) und VS (TP2) direkt aus 9–12 V. Nur USB mit Step-Up auf 12 V ist wegen 500 mA (2,5 W) vermutlich zu knapp.
  - **PCB:** KiCad, Fertigung z. B. JLCPCB/Aisler. Der Nutzer hat noch keine Platine entworfen, Claude führt Schritt für Schritt. Die Ader-zu-Pin-Zuordnung darf für ein gutes Routing neu gewählt werden; im Sketch ändern sich dann nur die Pin-Tabellen.
- **Frühere Planung (2026-10-12):**
  1. **Displays HDSP-2112** (empfohlen als Nächstes): Sie hängen an TP3 (5 V) und brauchen VS nicht. Sie bleiben dunkel, weil nichts hineingeschrieben wird.
     - P17 von VIN abziehen, `pin_tracer` hochladen, dann `pin_tracer.py COM8 +L26 dis1`. Gemessen werden alle 28 Lötpunkte von der Platinen-Rückseite (oben01–14, unten01–14, je von links); die Pin-Nummern leitet Claude aus den Treffern ab. Beim zweiten Display nur die abweichenden Pins (/CE, ggf. /RST).
     - Zur Erinnerung die HDSP-Pins: /RST 1, /FL 2, A0–A3 3–6, A4 10, /CLS 11, CLK 12, /WR 13, /CE 17, /RD 18, D0–D7 19, 20, 23–28. D0–D7 laufen vermutlich über IC4 (74HC245, A-Seite P46–P51 + zwei unbekannte, /OE = P26), deshalb mit `+L26` messen. IC4 Pin 8/9 (A7/A8) hängen an keiner Ader und an keiner Rail.
     - Erster Test danach: das Control Word mit D6 = 1 schreiben. Das startet den Selbsttest, und alle Punkte leuchten. Danach Text schreiben, z. B. die zuletzt gedrückte Taste wie in der GUI.
  2. **Drehregler (Encoder)** an D19/D20 (Pins: 1 GND, 3 A, 4 +5 V, 5 B), z. B. zum Drehen der Kette bzw. der Bars.
  3. **Ziel USB-HID bzw. MIDI**, erst wenn Display und Encoder laufen. Der Mega 2560 kann kein natives USB-HID. Optionen: HID-Firmware für den 16U2, ein Pro Micro oder Leonardo als Bridge, oder eine PC-seitige Software (seriell → MIDI/Tastatur).
