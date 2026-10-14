# PINOUT – BSMS/2 Keyboard-Display-Board

## Nomenklatur

- **P01–P64** = Ader des 64-poligen Flachbandkabels zwischen Display-Board und Controller-Board.
- **P01 = Ader mit roter Markierung.** Bei IDC-Steckern ist das gleichzeitig Pin 1 (Dreieck ▲ auf dem Stecker).
- Ungerade Pins (P01, P03, …) liegen in der einen Reihe des Steckers, gerade Pins (P02, P04, …) in der anderen. P01 und P02 liegen am selben Ende.
- Board-seitig: Leiterbahnen des Display-Boards heißen nach IC und Pin, z. B. `IC10.14` = Pin 14 von IC10.

## Arduino-Mega-Zuordnung (Stand 2026-10-09, blockweise)

Die Zuordnung ist so gewählt, dass die **ungerade Reihe** und die **gerade Reihe** des IDC-Steckers jeweils in 8er-Blöcken auf zusammenhängende Mega-Pins gehen:

| Block | Adern (Stecker-Reihe) | Mega |
|---|---|---|
| U1 | P01–P15 ungerade | P01 → **GND**, P11 → D32, Rest frei |
| U2 | P17–P31 ungerade | P17 → D38, P19–P23 frei, P25/27/29/31 → D46/48/50/52 |
| U3 | P33–P47 ungerade | A0–A7 |
| U4 | P49–P63 ungerade | P49–P59 → D2–D7, P61 → D15, P63 frei |
| G1 | P02–P16 gerade | D23, D25, …, D37 (ungerade Reihe der Doppelleiste) |
| G2 | P18–P32 gerade | D39, D41, …, D53 |
| G3 | P34–P48 gerade | A8–A15 |
| G4 | P50–P64 gerade | P50–P58 → D8–D12, P60 → D14, P62/P64 frei |

Bewusst frei gelassen:
- D0/D1: USB-Seriell.
- D13: onboard-LED, kann die Leitung belasten.
- D19, D20, D21: interruptfähig, reserviert für den Encoder (Ch A → D19, Ch B → D20).

Masse- und Versorgungsadern kommen **nicht** an I/O-Pins, sondern an GND bzw. 5V. 

Bisher gefundene Rails (Messungen 2026-10-09, Durchgang):
- **JU1** = P01, P03, P05, P07, P09. So heißt die Gruppe auf dem Board, Messpunkt vorne. Untereinander kurzgeschlossen und **mit 74HC595 Pin 8 verbunden → GND (bestätigt)** → Mega **GND**. D22, D24, D26, D28, D30 bleiben frei.
- **JU2** = P61, P62, P63, P64. So heißt die Gruppe auf dem Board. Untereinander kurzgeschlossen. Kein Durchgang zu 74HC595 Pin 8/16 und zu keinem Pin von IC1 (UDN2982A). **Durchgang zu IC3 Pin 9 = GND des ULN2804A.** Damit ist JU2 vermutlich ein **separater Leistungs-GND** für die LED-Ströme, auf dem Display-Board getrennt von JU1 (Logik-GND). Vermutlich werden beide erst auf dem Controller-Board verbunden. Prüfen: Durchgang JU1↔JU2 an der Steckerleiste J3 des Controller-Boards. Bis dahin für den Scan nur P61 als Eingang an D15, für den versorgten Betrieb später an GND.
- **TP3** (Messpunkt vorne auf dem Board, unten rechts) = P11, P13, P15, untereinander verbunden.
- **TP2** (Messpunkt vorne, unten links) = P17, P19, P21, P23, untereinander verbunden.
- **TP1** (Messpunkt vorne, links bei R1–R11) = nur P52, also eine einzelne Signalleitung.
- Muster: Die Rails liegen alle in der Reihe der ungeraden Adern (JU1 P01–09, TP3 P11–15, TP2 P17–23). Die Logik-Versorgung ist P60, siehe unten.
- **P60 = VCC der 74HC595 (IC10 Pin 16) → +5 V Logik (bestätigt).** Das ist eine einzelne Ader. Für den Scan bleibt sie an D14, für den versorgten Betrieb kommt sie an 5V.
- TP2 und TP3 sind damit nicht die Logik-Versorgung.
- Messung 2026-10-09 (Durchgang):
  - **TP2 piept an IC5 Pin 9 und IC1 Pin 9** (UDN2982A VS) → **TP2 = LED-Versorgung VS (bestätigt)**. Im Original vermutlich 12 V.
  - **TP3 hat keinen Durchgang** zu IC5.9, IC1.9, zu irgendeinem Pin von IC3 und IC6–IC9 oder zu P60. Funktion offen. Kandidat: Versorgung der HDSP-2112-Displays (Pin 14 VDD oder Pin 15 GND Versorgung).
  - Auch TP2 hat keinen Durchgang zu IC3. **COM des ULN2804A (IC3 Pin 10) liegt weder auf TP2 noch auf TP3.**
- Messung 2026-10-09: **P60 hat keinen Durchgang zu IC2 (74HC05) und IC4 (74HC245)**. **JU1 ist verbunden mit IC2 Pin 7 und IC4 Pin 10** (deren GND).
  - → IC2/IC4 haben eine **andere VCC als P60**. **TP3 piept an IC2 Pin 14 und IC4 Pin 20 → TP3 = zweite +5-V-Schiene (bestätigt)** für IC2, IC4 und die Displays HDSP-2112.
  - TP3 piept an den Displays: DIS1 untere Reihe, von hinten von links gezählt, Pins 5, 6, 7. DIS2 genauso, zusätzlich Pin 3. Die Zuordnung zur HDSP-Pinnummer ist noch offen (Einbaulage unklar).
- **TP1 (P52) hat ca. 0,7 kΩ nach JU1/GND**, also einen Widerstand nach Masse (Pull-down oder Kennung).
- Tasten haben **keinen** Kontakt nach JU1 oder JU2. Das passt zur Matrix laut Blockschaltbild: Tasten-Zeilenauswahl gegen Tasten-Spalten-Sense, nicht gegen GND.

**Kurzgeschlossene Gruppen nur einmal anklemmen:** JU1 → nur P01 an GND · TP3 → nur P11 · TP2 → nur P17 · JU2 → nur P61. Die übrigen Adern dieser Gruppen bleiben frei.

## Verdrahtung „versorgt“ (ab 2026-10-09, für `line_state`)

| Ader | Mega |
|---|---|
| P01 (JU1, GND Logik) | GND |
| P61 (JU2, GND ULN2804A) | GND |
| P60 (+5 V, 74HC595) | 5V |
| P11 (TP3, +5 V IC2/IC4/Displays) | 5V |
| P17 (TP2, LED-Versorgung VS) | bleibt an D38, der Pin wird von keinem Sketch benutzt (hochohmig) |
| alle anderen | wie in der Blocktabelle oben |

## IC-Pin-Zuordnung (Durchgangsmessung 2026-10-09, Board stromlos)

Rohdaten so, wie sie gemeldet wurden: IC-Pin → Mega-Pin → Ader.

**IC2 (SN74HC05):** Die erste Zählung des Nutzers war richtig. Die „180°-Korrektur“ von zwischendurch war falsch und ist durch den Matrix-Scan widerlegt. Die Kabeladern liegen an den **Eingängen**:

| IC2-Pin | Funktion | Ader | Matrix |
|---|---|---|---|
| 1 | 1A | P14 (D35) | Zeile 1 |
| 3 | 2A | P12 (D33) | Zeile 2 |
| 5 | 3A | P10 (D31) | Zeile 3 |
| 9 | 4A | P08 (D29) | Zeile 4 |
| 11 | 5A | P06 (D27) | Zeile 5 |
| 13 | 6A | P04 (D25) | Zeile 6 |

- Die Eingänge haben **Pull-downs** auf dem Board, deshalb sind sie im Ruhezustand LOW.
- **HIGH treiben = Zeile aktiv:** Der Open-Drain-Ausgang zieht dann die Tastenzeile auf GND.
- Ungeklärt ist, warum der Nutzer GND und VCC zweimal vertauscht gemessen hat (7 an JU1 bzw. 14 an TP3, später umgekehrt).

## Tastenmatrix (bestätigt 2026-10-09, `logs/matrix_finder_2026-10-09_163051.json`)

- Zeilen sind aktiv HIGH (74HC05-Eingänge).
- Spalten sind aktiv LOW, mit Pull-ups auf dem Board.

| Zeile ↓ / Spalte → | **P45** (A6) | **P44** (A13) | **P43** (A5) | **P42** (A12) | **P41** (A4) | **P40** (A11) |
|---|---|---|---|---|---|---|
| **P14** (D35) | SW1 LIFT ON-OFF | SW2 SPIN RATE | SW3 SPIN ON-OFF | – | – | – |
| **P12** (D33) | SW6 FIELD | SW7 LOCK PHASE | SW8 LOCK POWER | SW9 LOCK GAIN | SW10 SWEEP | SW11 SWEEP AMPL. |
| **P10** (D31) | SW12 HE LEVEL | SW13 (leer) | SW14 LOCK ON-OFF | SW15 AUTO LOCK | SW16 LOCK DC | SW17 LOCK SHIFT |
| **P08** (D29) | SW18 Z0 | SW19 Z1 | SW20 Z2 | SW21 Z3 | SW22 Z4 | SW23 Z5 |
| **P06** (D27) | SW24 X | SW25 Y | SW26 X²-Y² | SW27 XY | SW28 X³ | SW29 Y³ |
| **P04** (D25) | SW30 ONAXIS | SW31 AUTO SHIM | SW32 DIFF. MODE | SW33 STD BY | SW34 FINE | SW35 2nd |

Direkt verdrahtet, aktiv LOW: **P59 (D7) = SW4 CHAN. SELECT**, **P58 (D12) = SW5 TRANS. P-DOWN**.

**IC4 (SN74HC245):** 1 (DIR)→nichts (auf dem Board fest verdrahtet), 2 (A1)→A14 (P46), 3 (A2)→A7 (P47), 4 (A3)→A15 (P48), 5 (A4)→D2 (P49), 6 (A5)→D8 (P50), 7 (A6)→D3 (P51), 8 (A7)→nicht gemessen, 9 (A8)→D47 (P26) ⚠, 10→GND ✔, 19 (/OE)→D47 (P26), 20→5V ✔
- A1–A6 liegen auf P46–P51 der Reihe nach. **Hypothese: Datenbus D0–D7 der HDSP-2112** über IC4 gepuffert.
- **P26 = /OE von IC4**: in Ruhe HIGH, der Puffer ist also gesperrt.
- ⚠ Pin 9 → D47 ist vermutlich ein Messfehler, weil P26 schon /OE ist. Erwartet wird P52/P53. Pin 8 fehlt noch.

**IC10 (SN74HC595):** 11 (SRCLK)→D11 (P56), 12 (RCLK)→D47 (P26) ⚠ **falsch, richtig P57**, 13 (/OE)→nichts **falsch, richtig P26**, 14 (SER)→5V ⚠, 10 (/SRCLR)→5V ✔
- **P56 = Schiebetakt CLK** der LED-Schieberegister.
- ⚠ Dass RCLK auf P26 liegt (gleiche Ader wie IC4 /OE), ist unplausibel, bitte nachmessen.
- ⚠ SER an 5V ist nur plausibel, wenn IC10 nicht das erste Glied der Kette ist. Den Daten-Eingang (DLED) am ersten 595 suchen: SER des Chips, dessen Pin 14 nicht an einem QH' (Pin 9) eines anderen hängt.

## Messtabelle (stromlos, Controller-Board abgesteckt)

- **Diode zu GND:** Multimeter im Diodentest, schwarze Spitze auf GND, rote auf den Pin. Spannung notieren (`0` = Kurzschluss, `OL` = offen).
- **Diode von GND:** rote Spitze auf GND, schwarze auf den Pin.
- **Verbunden mit:** Durchgang zu IC-Pins, z. B. `IC10.11`.

| Pin | Arduino | Diode zu GND | Diode von GND | Verbunden mit | Funktion (vermutet/bestätigt) |
|---|---|---|---|---|---|
| P01 | **GND** | | | JU1, = 74HC595 Pin 8 | **GND (bestätigt)** |
| P02 | D23 | | | | |
| P03 | – (frei) | | | JU1, = 74HC595 Pin 8 | **GND (bestätigt)** |
| P04 | D25 | | | IC2 (74HC05) Eingang | **Tastenmatrix Zeile 6 (aktiv HIGH)** |
| P05 | – (frei) | | | JU1, = 74HC595 Pin 8 | **GND (bestätigt)** |
| P06 | D27 | | | IC2 (74HC05) Eingang | **Tastenmatrix Zeile 5 (aktiv HIGH)** |
| P07 | – (frei) | | | JU1, = 74HC595 Pin 8 | **GND (bestätigt)** |
| P08 | D29 | | | IC2 (74HC05) Eingang | **Tastenmatrix Zeile 4 (aktiv HIGH)** |
| P09 | – (frei) | | | JU1, = 74HC595 Pin 8 | **GND (bestätigt)** |
| P10 | D31 | | | IC2 (74HC05) Eingang | **Tastenmatrix Zeile 3 (aktiv HIGH)** |
| P11 | D32 | | | TP3 | **+5 V #2** (IC2, IC4, Displays; bestätigt) |
| P12 | D33 | | | IC2 (74HC05) Eingang | **Tastenmatrix Zeile 2 (aktiv HIGH)** |
| P13 | – (frei) | | | TP3 | **+5 V #2** (IC2, IC4, Displays; bestätigt) |
| P14 | D35 | | | IC2 (74HC05) Eingang | **Tastenmatrix Zeile 1 (aktiv HIGH)** |
| P15 | – (frei) | | | TP3 | **+5 V #2** (IC2, IC4, Displays; bestätigt) |
| P16 | D37 | | | DIS1.17 | **Display /CE** (pin_tracer 2026-10-13) |
| P17 | D38 | | | TP2 | LED-Versorgung VS (bestätigt) |
| P18 | D39 | | | DIS2.17 | **Display DIS2 /CE** (pin_tracer 2026-10-13) |
| P19 | – (frei) | | | TP2 | Rail, Funktion offen |
| P20 | D41 | | | DIS1.2 | **Display /FL** (pin_tracer 2026-10-13) |
| P21 | – (frei) | | | TP2 | Rail, Funktion offen |
| P22 | D43 | | | DIS1.3 | **Display A0** (pin_tracer 2026-10-13) |
| P23 | – (frei) | | | TP2 | Rail, Funktion offen |
| P24 | D45 | | | | |
| P25 | D46 | | | DIS1.4 | **Display A1** (pin_tracer 2026-10-13) |
| P26 | D47 | | | | |
| P27 | D48 | | | DIS1.5 | **Display A2** (pin_tracer 2026-10-13) |
| P28 | D49 | | | DIS1.28 | **Display D7** (pin_tracer 2026-10-13) |
| P29 | D50 | | | DIS1.6 | **Display A3** (pin_tracer 2026-10-13) |
| P30 | D51 | | | DIS1.27 | **Display D6** (pin_tracer 2026-10-13) |
| P31 | D52 | | | DIS1.10 | **Display A4** (pin_tracer 2026-10-13) |
| P32 | D53 | | | | |
| P33 | A0 | | | DIS1.26 | **Display D5** (pin_tracer 2026-10-13) |
| P34 | A8 | | | DIS1.25 | **Display D4** (pin_tracer 2026-10-13) |
| P35 | A1 | | | DIS1.24 | **Display D3** (pin_tracer 2026-10-13) |
| P36 | A9 | | | DIS1.23 | **Display D2** (pin_tracer 2026-10-13) |
| P37 | A2 | | | DIS1.20 | **Display D1** (pin_tracer 2026-10-13) |
| P38 | A10 | | | DIS1.19 | **Display D0** (pin_tracer 2026-10-13) |
| P39 | A3 | | | DIS1.1 | **Display /RST** (pin_tracer 2026-10-13) |
| P40 | A11 | | | Pull-up auf Board | **Tastenmatrix Spalte 6 (aktiv LOW)** |
| P41 | A4 | | | Pull-up auf Board | **Tastenmatrix Spalte 5 (aktiv LOW)** |
| P42 | A12 | | | Pull-up auf Board | **Tastenmatrix Spalte 4 (aktiv LOW)** |
| P43 | A5 | | | Pull-up auf Board | **Tastenmatrix Spalte 3 (aktiv LOW)** |
| P44 | A13 | | | Pull-up auf Board | **Tastenmatrix Spalte 2 (aktiv LOW)** |
| P45 | A6 | | | Pull-up auf Board | **Tastenmatrix Spalte 1 (aktiv LOW)** |
| P46 | A14 | | | | |
| P47 | A7 | | | | |
| P48 | A15 | | | | |
| P49 | D2 | | | | |
| P50 | D8 | | | | |
| P51 | D3 | | | | |
| P52 | D9 | | | TP1 | ca. 0,7 kΩ nach GND |
| P53 | D4 | | | DIS1.18 | **Display /RD** (pin_tracer 2026-10-13) |
| P54 | D10 | | | DIS1.13 | **Display /WR** (pin_tracer 2026-10-13) |
| P55 | D5 | | | | |
| P56 | D11 | | | | |
| P57 | D6 | | | | |
| P58 | D12 | | | Taste TRANS. P-DOWN (SW5) nach GND | **Taste TRANS. P-DOWN, aktiv LOW (bestätigt)** |
| P59 | D7 | | | Taste CHAN. SELECT (SW4) nach GND | **Taste CHAN. SELECT, aktiv LOW (bestätigt)** |
| P60 | D14 (Scan) → später **5V** | | | IC10.16 (74HC595 VCC) | **+5 V Logik (bestätigt)** |
| P61 | D15 | | | JU2 = IC3.9 (ULN2804A GND) | **Leistungs-GND der LED-Senken** (vermutet) |
| P62 | – (frei) | | | JU2 = IC3.9 (ULN2804A GND) | **Leistungs-GND der LED-Senken** (vermutet) |
| P63 | – (frei) | | | JU2 = IC3.9 (ULN2804A GND) | **Leistungs-GND der LED-Senken** (vermutet) |
| P64 | – (frei) | | | JU2 = IC3.9 (ULN2804A GND) | **Leistungs-GND der LED-Senken** (vermutet) |

## Displays HDSP-2112 (2026-10-13, `logs/pin_tracer_2026-10-13_085802.json`, Basis P26 LOW)

DIS1 vollständig gemessen (Lötpunkte Rückseite, je Reihe von links). Alle 28 Punkte passen exakt aufs HDSP-2112-Pinout:
Reihe **oben** = Pin 15→28, Reihe **unten** = Pin 14→1. Alle Signale **direkt** am Kabel, nicht über IC4 (74HC245).

| HDSP-Pin | Signal | Ader | Mega | Lötpunkt |
|---|---|---|---|---|
| 1 | /RST | P39 | A3 | unten14 |
| 2 | /FL | P20 | D41 | unten13 |
| 3 | A0 | P22 | D43 | unten12 |
| 4 | A1 | P25 | D46 | unten11 |
| 5 | A2 | P27 | D48 | unten10 |
| 6 | A3 | P29 | D50 | unten09 |
| 7–9 | NC | – | – | unten08–06 |
| 10 | A4 | P31 | D52 | unten05 |
| 11 | /CLS | fest LOW | – | unten04 (DIS1 = Slave, bekommt Takt von außen) |
| 12 | CLK | keine Ader | – | unten03 (vermutlich vom CLK-Ausgang von DIS2, dem Master) |
| 13 | /WR | P54 | D10 | unten02 |
| 14 | VDD | +5 V | – | unten01 |
| 15/16 | GND | GND | – | oben01/02 |
| 17 | /CE | P16 | D37 | oben03 |
| 18 | /RD | P53 | D4 | oben04 |
| 19 | D0 | P38 | A10 | oben05 |
| 20 | D1 | P37 | A2 | oben06 |
| 21–22 | NC | – | – | oben07/08 |
| 23 | D2 | P36 | A9 | oben09 |
| 24 | D3 | P35 | A1 | oben10 |
| 25 | D4 | P34 | A8 | oben11 |
| 26 | D5 | P33 | A0 | oben12 |
| 27 | D6 | P30 | D51 | oben13 |
| 28 | D7 | P28 | D49 | oben14 |

**DIS2** (`logs/pin_tracer_2026-10-13_090319.json`): gleiche Lage, gleiche Adern (Kontrolle: D0 = P38, /WR = P54, /RST = P39),
außer **/CE = P18 (D39)**. **/CLS = HIGH**, also ist DIS2 der Takt-Master, sein CLK-Ausgang speist DIS1 (keine Kabelader).

**Lage auf der Front:** DIS2 links, DIS1 rechts (wie Platinen-Beschriftung); Zeichenposition 0 = links. Schreibtest mit `display_test` erfolgreich.

Noch freie Adern ohne Funktion: **P24 (D45), P32 (D53)**.

## Encoder (S1-256-BM6)

| Encoder-Pin | Funktion | Arduino |
|---|---|---|
| 1 | GND | GND |
| 2 | NC | – |
| 3 | Ch A | D19 |
| 4 | +5 V | **D20 (OUTPUT HIGH, Notlösung)**, besser 5V bzw. ICSP Pin 2 |
| 5 | Ch B | **D21** |

Stand 2026-10-14: Es war keine 5-V-Buchse mehr frei, deshalb versorgt der Sketch `keys` den Encoder über D20. Ch B liegt auf D21 (auch interruptfähig). Gemessen: 4,6 V am Encoder (Pin 4) bei Versorgung über D20. Für eine eigene Platine eine richtige 5-V-Versorgung vorsehen.

## LED-Matrix (2026-10-12, TP2 = VS an VIN ca. 8,6 V)

- **High-Side (Spalten):** IC1 (UDN2982A) Pin 1–6 ← B-Seite von IC4 (74HC245). Die A-Seite ist der Bus P46–P51. **P26 = /OE von IC4**; nur bei P26 = LOW kommen die Spaltenmuster bei IC1 an. In Ruhe liegt der Bus auf HIGH, dann sind alle Spalten an.
- **Low-Side (Zeilen):** IC3 (ULN2804A) wird von den **Tasten-Zeilen** angesteuert (gemessen: IC3.7 ← P04). Zeile HIGH = LED-Zeile aktiv.

| Zeile (HIGH) | LEDs |
|---|---|
| P14 | SW1, SW2, SW3 |
| P12 | SW6–SW11 |
| P10 | SW12–SW17 |
| P08 | SW18–SW23 |
| P06 | SW24–SW29 |
| P04 | SW30–SW35 |
| P02 | LED20 rot, LED21 grün, LED22 rot, LED23 rot, LED24 rot (LED27 unbestückt) |

- **Spalten** (`logs/led_finder_2026-10-12_075909.json`): **P46 (A14) = Spalte 1, P47 (A7) = 2, P48 (A15) = 3, P49 (D2) = 4, P50 (D8) = 5, P51 (D3) = 6**. Getestet in Zeile P04: P46 ONAXIS, P47 AUTO SHIM, P48 DIFF. MODE, P49 STD BY, P50 FINE, P51 2nd. Die Reihenfolge ist also dieselbe wie bei den Tasten (SW-Nummer aufsteigend).
- **Eine LED einschalten:** Zeile HIGH, Spalten-Ader HIGH (alle anderen Spalten LOW), P26 LOW. Multiplexen: immer nur eine Zeile gleichzeitig.
- **Status-LEDs in Zeile P02** (`logs/led_finder_2026-10-12_155548.json`): P46 = LED20 (rot), P47 = LED21 (grün), P48 = LED22 (rot), P49 = LED27 (nicht bestückt; Lötpads zwischen LED22 und LED23, per Multimeter bestätigt), P50 = LED23 (rot), P51 = LED24 (rot). Die Tasten CHAN. SELECT und TRANS. P-DOWN (Direkttasten) haben in Zeile P14 keine sichtbare LED.
- Bargraph LED25: bei P26 = LOW leuchten 6 von 10 Segmenten, bei zusätzlich P57 = LOW nur noch 2. Noch ungeklärt.

## Router-Display: 74HC595-Kette IC10–IC14 (2026-10-12, `logs/pin_tracer_2026-10-12_081150.json`)

| 595-Pin | Funktion | Ader (alle fünf ICs gemeinsam) |
|---|---|---|
| 11 | SRCLK (Schiebetakt, steigende Flanke) | **P56** (D11) |
| 12 | RCLK (Übernahme ins Ausgangsregister, steigende Flanke) | **P57** (D6) |
| 13 | /OE (Ausgänge frei bei LOW) | **P26** (D47), dieselbe Ader wie /OE des 74HC245 (IC4) |
| 10 | /SRCLR (Löschen bei LOW) | **P55** (D5) |
| 14 | SER | IC10: keine Kabelader (Ruhe HIGH); IC11–IC14: Ruhe LOW, kein Treffer → Kaskade von QH' des Vorgängers |

- **IC10 ist der Anfang der Kette.** Die frühere Messung IC10.12 → P26 war falsch (RCLK ist P57); die Messung IC10.13 → nichts ebenfalls (/OE ist P26).
- Folge von P26 = gemeinsames /OE: Jede Freigabe der Tasten-LED-Spalten schaltet auch die Router-LEDs frei. Das erklärt den leuchtenden Bargraph LED25 bei P26 = LOW, ebenso die Änderung bei P57 LOW (Übernahme neuer Registerinhalte).
- ~~Hypothese~~ (widerlegt, SER liegt fest an +5 V): SER von IC10 an B-Seite des 74HC245 (B7 oder B8). Dann ist der Dateneingang nur bei P26 = LOW mit einer Kabelader (A7/A8) verbunden. Test: `pin_tracer.py COM8 +L26 IC4.8 IC4.9 IC10.14`.
- Ergebnis (`logs/pin_tracer_2026-10-12_081754.json`, P26 LOW gehalten): IC4.8 (A7) und IC4.9 (A8): Ruhe LOW, keine Ader. IC4.11 (B8) → P56, IC4.12 (B7) → P57 (?). IC10.14 (SER): Ruhe LOW, keine Ader (bei P26 HIGH war sie HIGH). Widersprüchlich bzw. unklar, ggf. Zählfehler an IC4. Weiter funktional mit `tools/router_finder.py`.

### Reihenfolge in der Kette (bestätigt mit `led_test`, 2026-10-12)

Nach Löschen (P55) und k Takten (P56) mit Übernahme (P57) leuchten die ersten k Positionen. Erster Lauf: `logs/router_finder_2026-10-12_082124.json` (dort war der Bargraph-Teil um 4 verzählt).

| Position | LED |
|---|---|
| 1–10 | Bargraph LED25 (grün), Segment 1–10 |
| 11–20 | Bargraph LED26 (rot), Segment 1–10 |
| 21 | LED5 |
| 22 | LED6 |
| 23 | LED14 |
| 24 | LED15 |
| 25 | LED1 |
| 26 | LED7 |
| 27 | LED4 |
| 28 | LED10 |
| 29 | LED13 |
| 30 | LED18 |
| 31 | LED in Taste TRANS. P-DOWN |
| 32 | LED19 |
| 33 | LED2 |
| 34 | LED8 |
| 35 | LED11 |
| 36 | LED16 |
| 37 | LED3 |
| 38 | LED9 |
| 39 | LED12 |
| 40 | LED17 |

- **SER von IC10 = TP3 (+5 V), fest verdrahtet** (Durchgang stromlos, 2026-10-12; die allererste Messung „IC10.14 → 5V“ war also richtig). IC4 Pin 8 und Pin 9 piepsen gegen keinen der Messpunkte JU1/TP3/TP2/JU2.
- Folge: Die Kette lässt sich nur **löschen und mit k Einsen füllen**. Beliebige Muster sind ohne Umbau nicht möglich. LED26 (rot) lässt sich also nur zusammen mit dem vollen LED25 einschalten. Die Bargraphs waren im Original Pegelanzeigen, dafür passt das.
- Matrix Zeile P14, Spalte 4–6: keine LED (bestätigt).

**Lage und Frontblenden-Beschriftung der Router-LEDs** (Fotos 2026-10-12; ⊓ = Pulssymbol):

| | links | Mitte | rechts |
|---|---|---|---|
| oben | LED1 OBS, LED2 ⊓, LED3 ⊓, LED4 MIS, LED5 X (Pfeil →) | Bargraph LED25 „FWD“ | LED6 H (Pfeil ←), LED7 OBS, LED8 ⊓, LED9 ⊓, LED10 MIS |
| Mitte | | | LED19 „ADC“, rot (zwischen den Bars und der rechten Gruppe) |
| unten | LED11 ⊓, LED12 ⊓, LED13 MIS, LED14 Y (Pfeil →) | Bargraph LED26 „REFL“ | LED15 Z (Pfeil ←), LED16 ⊓, LED17 ⊓, LED18 MIS |

Darunter auf der Blende: links das Fenster „SAMPLE / SPIN MEAS.“ (HDSP-Display), rechts ein zweites Fenster.
