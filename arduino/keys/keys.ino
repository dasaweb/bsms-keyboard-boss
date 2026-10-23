// keys – Tasten, Tasten-LEDs, Status-LEDs und 595-Kette des BSMS/2 Keyboard-Display-Boards
// (Board VERSORGT, fuer die LEDs TP2 = VS an VIN)
//
// Matrix (ermittelt 2026-10-09 mit tools/matrix_finder.py, siehe PINOUT.md):
//   6 Zeilen P14, P12, P10, P08, P06, P04 = Eingaenge des 74HC05 (IC2) und des ULN2804A (IC3),
//     mit Pull-down auf dem Board; Zeile aktiv = HIGH treiben.
//   6 Tasten-Spalten P45, P44, P43, P42, P41, P40, mit Pull-ups auf dem Board; Taste gedrueckt
//     in aktiver Zeile = LOW.
//   Direkt verdrahtet (LOW = gedrueckt): P59 = CHAN. SELECT (SW4), P58 = TRANS. P-DOWN (SW5).
// LEDs (ermittelt 2026-10-12 mit tools/led_finder.py und led_test):
//   LED-Zeilen = Tasten-Zeilen, dazu P02 = Status-LEDs (Spalte 1 LED20, 2 LED21, 3 LED22,
//   4 LED27 unbestueckt, 5 LED23, 6 LED24).
//   LED-Spalten = Bus P46-P51 (HIGH = an), ueber 74HC245 (IC4) auf UDN2982A (IC1);
//   P26 = /OE des 245 UND der 595-Kette. Multiplex: eine Zeile zur Zeit.
//   595-Kette: P60 = DLED (SER von IC10, an D14), P56 = SRCLK, P57 = RCLK, P55 = /SRCLR (bleibt HIGH).
//   40 Positionen, frei schaltbar (seit 2026-10-23, vorher hing P60 irrtuemlich an 5 V):
//   1-10 Bargraph LED25 gruen (FWD), 11-20 LED26 rot (REFL), 21-40 LED1-19 und die LED in TRANS. P-DOWN
//   (Reihenfolge siehe CHAIN_LED). Die Bars zeigen einen Pegel 0-20 (erst FWD, dann REFL).
//
// Tastenfunktionen (Demo):
//   ONAXIS (SW30) Pegel +1, 2nd (SW35) Pegel -1 (gehalten: Wiederholung nach 500 ms, dann alle 80 ms),
//   STD BY (SW33) Pegel 0 und alle Router-LEDs aus,
//   TRANS. P-DOWN (SW5) Lauflicht LED1-19 mit Bars (ihre LED leuchtet dabei),
//   CHAN. SELECT (SW4) Kanal weiterschalten: H, X, Y, Z, aus (Pfeil-LEDs LED6, LED5, LED14, LED15).
//   ONAXIS, 2nd und STD BY leuchten, solange sie gedrueckt sind.
//   HE LEVEL (SW12), leere Taste (SW13), LOCK ON-OFF (SW14), LOCK DC (SW16), LOCK SHIFT (SW17)
//   schalten LED20, LED21, LED22, LED23, LED24 blinkend an bzw. aus; ihre eigene LED leuchtet dazu.
//   FINE (SW34) schaltet die Display-Helligkeit eine Stufe heller (13-20-27-40-53-80-100 %, dann wieder 13 %;
//   gehalten: Wiederholung alle 400 ms). Der HDSP-2112 kennt nur diese 8 Stufen.
//   SWEEP (SW10) startet/stoppt die Display-Animation: ein Strichmaennchen laeuft pixelweise ueber die
//   16 Stellen (selbstdefinierte Zeichen UDC 0/1; im Spalt zwischen den Zeichen 2 Pixel unsichtbar),
//   danach eine Laufschrift, endlos. Jede andere Taste stoppt sie.
//   SWEEP AMPL. (SW11) stellt den Spalt fuer das Maennchen um (0-5 Pixel, zum Vergleichen) und startet es neu.
//   DIFF. MODE (SW32) schaltet die Uhr an/aus: "09:15:42  13.10." (Uhrzeit links, Datum rechtsbuendig) (Zeit kommt vom PC per Z-Befehl).
//   Alle anderen Tasten schalten ihre eigene LED um.
// Drehregler (optischer Encoder S1-256-BM6, 256 Striche, 4 Flanken = 1024 Schritte/Umdrehung):
//   Ch A = D19, Ch B = D21 (beide Interrupt), +5 V NOTLOESUNG ueber D20 (HIGH; keine 5-V-Buchse frei).
//   Drehen stellt den Pegel der Bars ein (ENC_DIV Schritte je Stufe).
//   Bei laufender SWEEP-Animation steuert das Rad stattdessen das Maennchen (ENC_MAN_DIV Schritte je Pixel,
//   rueckwaerts dreht es sich um; am Rand erscheint es auf der anderen Seite). SWEEP aus/an = wieder Automatik.
// Displays (2 x HDSP-2112, ermittelt 2026-10-13, siehe PINOUT.md): 16 Zeichen, links DIS2, rechts DIS1.
//   Zeigen beim Druecken den Tastennamen (mit AN/AUS), bei ONAXIS/2nd/Drehregler den Pegel.
//
// Ausgabe (115200 Baud): "D SWn" / "U SWn" beim Druecken/Loslassen, "L SWn 0|1" bei Tasten-LEDs,
//   "S <c> 0|1|2" bei Status-LEDs (Spalte c; 2 = blinkt), "B <hex>" bei Aenderung der Bar-Segmente
//   (Bit i = Kettenposition 1+i, also FWD 1-10 und REFL 11-20),
//   "R <hex>" bei Aenderung der Router-LEDs (Bit i = Kettenposition 21+i),
//   "E <zaehler>" bei Drehung (hoechstens alle 50 ms), "W <16 Zeichen>" bei jeder Aenderung des Display-Texts (Strichmaennchen als Zeichen 0x7F), "H <0-6>" bei Aenderung der Helligkeit.
// Befehle (mit Zeilenende):
//   Q          gedrueckte Tasten als "Q SWa SWb ..." ausgeben
//   K<n>       Tastendruck von SWn nachahmen (gleiche Wirkung wie die echte Taste)
//   L<n>=<0|1> LED der Taste SWn setzen
//   S<c>=<0|1|2> Status-LED in Spalte c setzen (0 aus, 1 an, 2 blinkt)
//   C<k>       Pegel der Bars auf k (0-20) setzen (die ersten k Segmente an, der Rest aus)
//   B<i>=<0|1> einzelnes Bar-Segment i setzen (1-10 FWD, 11-20 REFL)
//   R<n>=<0|1> Router-LED n setzen (1-19 = LED1-19, 0 = LED in TRANS. P-DOWN)
//   T<0|1>     Tastenfunktionen aus/an (Vorgabe an)
//   A<0|1>     alle LEDs aus/an
//   W<text>    Text auf die Displays (bis 16 Zeichen, links beginnend, UTF-8). Umlaute, ss und einige Emojis
//              (Smiley, traurig, Herz, Note, Stern) werden als selbstdefinierte Zeichen UDC 4-15 dargestellt.
//   Z<sek>     Uhr stellen: Ortszeit als Sekunden seit 1.1.1970 (die GUI schickt das beim Start und jede Minute)
//   H<0-7>     Display-Helligkeit (0 = 100 %, 3 = 40 %, 6 = 13 %, 7 = aus)

const uint8_t ROWS[7] = {35, 33, 31, 29, 27, 25, 23};      // P14 P12 P10 P08 P06 P04, P02 (nur LEDs)
const uint8_t COLS[6] = {A6, A13, A5, A12, A4, A11};       // P45 P44 P43 P42 P41 P40
const uint8_t BUS[6]  = {A14, A7, A15, 2, 8, 3};           // P46 P47 P48 P49 P50 P51
const uint8_t PIN_OE = 47;                                  // P26, /OE von 74HC245 und 595
const uint8_t PIN_CLR = 5, PIN_SRCLK = 11, PIN_RCLK = 6;    // P55, P56, P57
const uint8_t PIN_DLED = 14;                                // P60, SER von IC10
const uint8_t PIN_CHAN_SELECT = 7;                          // P59
const uint8_t PIN_TRANS_PDOWN = 12;                         // P58
const uint16_t HOLD_US = 1500;                              // Leuchtdauer je Zeile
const uint8_t CHAIN_LEN = 40, BAR_LEN = 20, ROUTER_LEN = 20;
// Kettenposition 21-40 -> LED-Nummer (0 = LED in Taste TRANS. P-DOWN), bestaetigt 2026-10-23
const uint8_t CHAIN_LED[ROUTER_LEN] = {5, 6, 14, 15, 1, 7, 4, 10, 13, 18, 0, 19, 2, 8, 11, 16, 3, 9, 12, 17};
const uint8_t CHANNEL_LED[4] = {6, 5, 14, 15};               // Pfeile H, X, Y, Z
const char CHANNEL_NAME[4] = {'H', 'X', 'Y', 'Z'};
const uint16_t BLINK_MS = 400;                              // halbe Blinkperiode der Status-LEDs
const uint16_t REPEAT_DELAY_MS = 500, REPEAT_MS = 80;     // Tastenwiederholung ONAXIS/2nd
const uint16_t REPEAT_FINE_MS = 400;
const uint8_t PIN_ENC_A = 19, PIN_ENC_B = 21, PIN_ENC_VCC = 20;
const uint8_t ENC_DIV = 24;                                // Encoder-Schritte je Kettenstufe
const uint8_t ENC_MAN_DIV = 8;                             // Encoder-Schritte je Pixel des Maennchens                        // Tastenwiederholung FINE (Helligkeit)

// Displays HDSP-2112: gemeinsamer Bus, eigenes /CE je Display
const uint8_t PIN_DRST = A3, PIN_DFL = 41, PIN_DWR = 10, PIN_DRD = 4;   // P39, P20, P54, P53
const uint8_t PIN_DCE[2] = {39, 37};                         // P18 = DIS2 (links), P16 = DIS1 (rechts)
const uint8_t DADDR[5] = {43, 46, 48, 50, 52};               // A0-A4 = P22, P25, P27, P29, P31
const uint8_t DDATA[8] = {A10, A2, A9, A1, A8, A0, 51, 49};  // D0-D7 = P38, P37, P36, P35, P34, P33, P30, P28
const char *const BRIGHT_PCT[8] = {"100", "80", "53", "40", "27", "20", "13", "0"};

// SW-Nummer je Zeile/Spalte, 0 = keine Taste
const uint8_t SW[6][6] = {
  { 1,  2,  3,  0,  0,  0},
  { 6,  7,  8,  9, 10, 11},
  {12, 13, 14, 15, 16, 17},
  {18, 19, 20, 21, 22, 23},
  {24, 25, 26, 27, 28, 29},
  {30, 31, 32, 33, 34, 35},
};

const uint8_t SW_CHAN_SELECT = 4, SW_TRANS_PDOWN = 5, SW_ONAXIS = 30, SW_STDBY = 33, SW_FINE = 34, SW_2ND = 35, SW_SWEEP = 10, SW_SWEEP_AMPL = 11, SW_CLOCK = 32;
// Tastennamen fuer das Display (Index = SW-Nummer)
const char *const KEY_NAME[36] = {"",
  "LIFT ON-OFF", "SPIN RATE", "SPIN ON-OFF", "CHAN. SELECT", "TRANS. P-DOWN",
  "FIELD", "LOCK PHASE", "LOCK POWER", "LOCK GAIN", "SWEEP", "SWEEP AMPL.",
  "HE LEVEL", "(LEER)", "LOCK ON-OFF", "AUTO LOCK", "LOCK DC", "LOCK SHIFT",
  "Z0", "Z1", "Z2", "Z3", "Z4", "Z5",
  "X", "Y", "X2-Y2", "XY", "X3", "Y3",
  "ONAXIS", "AUTO SHIM", "DIFF. MODE", "STD BY", "FINE", "2nd"};
// Tasten fuer die Status-LEDs und ihre Spalte (1-6)
const uint8_t STATUS_KEYS[5][2] = {{12, 1}, {13, 2}, {14, 3}, {16, 5}, {17, 6}};

const uint8_t NKEYS = 36;          // Index = SW-Nummer
bool stable[NKEYS], last[NKEYS];
uint8_t ledRow[7];                 // Bit c = LED in Spalte c an
uint8_t statusBlink;               // Bit c = Status-LED in Spalte c blinkt (wenn an)
bool demo = true;
String cmd;

uint8_t barLevel = 0;              // eingestellter Pegel der Bars (0-20)
uint32_t barBits = 0;              // Bit i = Bar-Segment 1+i (normal die ersten barLevel Bits)
uint32_t routerBits = 0;           // Bit i = Kettenposition 21+i
uint32_t shownBar = 0xFFFFFFFF;    // aktuell angezeigt (auch waehrend der Animation)
uint32_t shownBits = 0xFFFFFFFF;
bool animOn = false;               // Lauflicht von TRANS. P-DOWN
int8_t animStep = 0, animDir = 0;  // animStep = LED-Nummer 0-19 (0 = vor LED1)
uint8_t channel = 0;               // 0 = aus, 1-4 = H, X, Y, Z
unsigned long animNext = 0;
uint8_t repeatKey = 0;             // gehaltene Taste mit Wiederholung (ONAXIS/2nd), 0 = keine
unsigned long repeatNext = 0;
char dispText[17];                 // aktueller Display-Text, 16 Zeichen
uint8_t brightness = 0;            // 100 %

// SWEEP-Animation
const uint8_t MAN[4][7] = {        // 4 Lauf-Phasen, 5 Pixel breit (Bit 4 = links)
  {0b00100, 0b01110, 0b10101, 0b00100, 0b01010, 0b01010, 0b10001},
  {0b00100, 0b00100, 0b01110, 0b00100, 0b00100, 0b01100, 0b01010},
  {0b00100, 0b01110, 0b10101, 0b00100, 0b01010, 0b10010, 0b10001},
  {0b00100, 0b00100, 0b01110, 0b00100, 0b00100, 0b00110, 0b01010},
};
const char SWEEP_TEXT[] = "*** BRUKER BSMS/2 ***";
uint8_t pitch = 8;                 // virtuelle Pixel je Zeichen: 5 sichtbar + Spalt (Vorgabe 3, SWEEP AMPL. 0-5)
const uint16_t MAN_MS = 70, SCROLL_MS = 180;
bool sweepOn = false;
uint8_t sweepPhase;                // 0 = Maennchen, 1 = Laufschrift
int sweepPos;
unsigned long sweepNext;
uint8_t udcMain = 0;               // welches UDC-Zeichen die GUI als Maennchen zeigt
uint8_t udcBuf = 0;                // Doppelpuffer: UDC 0/1 oder 2/3
bool sweepManual = false;          // Maennchen wird vom Drehrad gesteuert
bool manLeft = false;              // Maennchen schaut nach links (gespiegelt)

// Encoder
volatile long encCount = 0;
volatile uint8_t encState = 0;
long encBase = 0, encReported = 0;
unsigned long encReportNext = 0;
const int8_t QDEC[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void encISR() {
  uint8_t st = digitalRead(PIN_ENC_A) << 1 | digitalRead(PIN_ENC_B);
  encCount -= QDEC[encState << 2 | st];   // Vorzeichen so, dass rechts drehen = mehr (getestet 2026-10-14)
  encState = st;
}

// Uhr (Zeit vom PC)
bool clockOn = false, timeValid = false;
unsigned long syncEpoch, syncMillis;
unsigned long clockShown = 0xFFFFFFFF;

// Feste Sonderzeichen in UDC 4-15 (5x7, Bit 4 = links): Zeichen fuer die GUI (UTF-8) und Bitmuster
const char *const GLYPH_UTF8[12] = {"Ä", "Ö", "Ü", "ä", "ö", "ü", "ß", "☺", "☹", "♥", "♪", "★"};
const uint8_t GLYPH[12][7] = {
  {0b01010, 0b00000, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001},   // Ä
  {0b01010, 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110},   // Ö
  {0b01010, 0b00000, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110},   // Ü
  {0b01010, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111},   // ä
  {0b01010, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110},   // ö
  {0b01010, 0b00000, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101},   // ü
  {0b01100, 0b10010, 0b10010, 0b10110, 0b10001, 0b10001, 0b10110},   // ß
  {0b00000, 0b01010, 0b01010, 0b00000, 0b10001, 0b01110, 0b00000},   // Smiley
  {0b00000, 0b01010, 0b01010, 0b00000, 0b01110, 0b10001, 0b00000},   // traurig
  {0b00000, 0b01010, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000},   // Herz
  {0b00100, 0b00110, 0b00101, 0b00100, 0b01100, 0b11100, 0b11000},   // Note
  {0b00100, 0b00100, 0b11111, 0b01110, 0b01010, 0b10001, 0b00000},   // Stern
};
// Unicode-Codepunkt -> Index in GLYPH
const struct { uint32_t cp; uint8_t g; } GLYPH_MAP[] = {
  {0xC4, 0}, {0xD6, 1}, {0xDC, 2}, {0xE4, 3}, {0xF6, 4}, {0xFC, 5}, {0xDF, 6},
  {0x263A, 7}, {0x1F600, 7}, {0x1F603, 7}, {0x1F604, 7}, {0x1F642, 7}, {0x1F60A, 7}, {0x1F601, 7},
  {0x2639, 8}, {0x1F641, 8}, {0x1F61E, 8}, {0x1F622, 8},
  {0x2764, 9}, {0x2665, 9}, {0x1F496, 9}, {0x1F499, 9},
  {0x266A, 10}, {0x266B, 10}, {0x1F3B5, 10}, {0x1F3B6, 10},
  {0x2605, 11}, {0x2B50, 11}, {0x1F31F, 11},
};

bool findKey(uint8_t sw, uint8_t &r, uint8_t &c) {
  for (r = 0; r < 6; r++)
    for (c = 0; c < 6; c++)
      if (SW[r][c] == sw) return true;
  return false;
}

bool ledOf(uint8_t sw) {
  uint8_t r, c;
  return findKey(sw, r, c) && (ledRow[r] >> c & 1);
}

void setLed(uint8_t sw, bool on) {
  uint8_t r, c;
  if (!findKey(sw, r, c)) return;
  bool was = ledRow[r] >> c & 1;
  if (on) ledRow[r] |= 1 << c; else ledRow[r] &= ~(1 << c);
  if (was != on) {
    Serial.print(F("L SW")); Serial.print(sw); Serial.println(on ? F(" 1") : F(" 0"));
  }
}

uint8_t statusMode(uint8_t col) {         // 0 aus, 1 an, 2 blinkt
  uint8_t bit = 1 << (col - 1);
  return !(ledRow[6] & bit) ? 0 : (statusBlink & bit) ? 2 : 1;
}

void setStatus(uint8_t col, uint8_t mode) {   // col 1-6
  if (col < 1 || col > 6 || mode > 2) return;
  uint8_t was = statusMode(col), bit = 1 << (col - 1);
  if (mode) ledRow[6] |= bit; else ledRow[6] &= ~bit;
  if (mode == 2) statusBlink |= bit; else statusBlink &= ~bit;
  if (was != mode) {
    Serial.print(F("S ")); Serial.print(col); Serial.print(' '); Serial.println(mode);
  }
}

void pulse(uint8_t pin) {
  digitalWrite(pin, LOW);
  delayMicroseconds(2);
  digitalWrite(pin, HIGH);
  delayMicroseconds(2);
}

int8_t routerBit(uint8_t led) {
  for (uint8_t i = 0; i < ROUTER_LEN; i++) if (CHAIN_LED[i] == led) return i;
  return -1;
}

// 40 Bits schieben (das zuerst geschobene landet auf Position 40) und uebernehmen
void showChain(uint32_t bar, uint32_t bits) {
  for (uint8_t pos = CHAIN_LEN; pos >= 1; pos--) {
    bool on = pos <= BAR_LEN ? bar >> (pos - 1) & 1 : bits >> (pos - BAR_LEN - 1) & 1;
    digitalWrite(PIN_DLED, on);
    pulse(PIN_SRCLK);
  }
  digitalWrite(PIN_DLED, LOW);
  pulse(PIN_RCLK);
  if (bar != shownBar) {
    shownBar = bar;
    Serial.print(F("B ")); Serial.println(bar, HEX);
  }
  if (bits != shownBits) {
    shownBits = bits;
    Serial.print(F("R ")); Serial.println(bits, HEX);
  }
}

void updateChain() {
  if (!animOn) { showChain(barBits, routerBits); return; }
  uint32_t bits = 1UL << routerBit(0);                 // LED in TRANS. P-DOWN leuchtet mit
  if (animStep > 0) bits |= 1UL << routerBit(animStep);
  showChain((1UL << (animStep * BAR_LEN + 10) / 19) - 1, bits);
}

void setLevel(int k) {
  animOn = false;
  barLevel = constrain(k, 0, BAR_LEN);
  barBits = (1UL << barLevel) - 1;
  updateChain();
}

void setBar(uint8_t seg, bool on) {
  if (seg < 1 || seg > BAR_LEN) return;
  animOn = false;
  if (on) barBits |= 1UL << (seg - 1); else barBits &= ~(1UL << (seg - 1));
  updateChain();
}

void setRouter(uint8_t led, bool on) {
  int8_t i = routerBit(led);
  if (i < 0) return;
  if (on) routerBits |= 1UL << i; else routerBits &= ~(1UL << i);
  updateChain();
}

void nextChannel() {
  channel = (channel + 1) % 5;
  for (uint8_t i = 0; i < 4; i++) setRouter(CHANNEL_LED[i], channel == i + 1);
  char t[17];
  if (channel) snprintf(t, sizeof(t), "KANAL %c", CHANNEL_NAME[channel - 1]);
  else strcpy(t, "KANAL AUS");
  showText(t);
}

void startAnim() {
  animOn = true;
  animStep = 0;
  animDir = 1;
  updateChain();
  animNext = millis();
}

void runAnim() {
  if (!animOn || millis() < animNext) return;
  animNext = millis() + 60;
  animStep += animDir;
  if (animStep >= 19) { animStep = 19; animDir = -1; }
  if (animStep <= 0 && animDir < 0) { animOn = false; }
  updateChain();
}

void dispWrite(uint8_t d, uint8_t addr, uint8_t data) {
  for (uint8_t i = 0; i < 5; i++) digitalWrite(DADDR[i], addr >> i & 1);
  for (uint8_t i = 0; i < 8; i++) digitalWrite(DDATA[i], data >> i & 1);
  digitalWrite(PIN_DCE[d], LOW);
  digitalWrite(PIN_DWR, LOW);
  delayMicroseconds(1);
  digitalWrite(PIN_DWR, HIGH);
  digitalWrite(PIN_DCE[d], HIGH);
}

// Text auf beide Displays (16 Zeichen, mit Leerzeichen aufgefuellt), meldet "W <text>"
void showText(const char *s) {
  char t[17];
  uint8_t n = 0;
  while (n < 16 && s[n]) { t[n] = s[n]; n++; }
  while (n < 16) t[n++] = ' ';
  t[16] = 0;
  if (!strcmp(t, dispText)) return;
  for (uint8_t i = 0; i < 16; i++)
    if (t[i] != dispText[i]) dispWrite(i / 8, 0x18 | (i % 8), t[i]);   // A4=1, A3=1: Zeichen-RAM
  strcpy(dispText, t);
  Serial.print(F("W "));
  for (uint8_t i = 0; i < 16; i++) {           // UDC-Zeichen fuer die GUI: 0x7F = Maennchen
    uint8_t c = dispText[i];
    if (c < 0x80) Serial.write(c);
    else if ((c & 0x0F) >= 4) Serial.print(GLYPH_UTF8[(c & 0x0F) - 4]);
    else Serial.write((c & 0x0F) == udcMain ? 0x7F : ' ');
  }
  Serial.println();
}

// UTF-8-Text in Display-Codes umsetzen (Sonderzeichen -> UDC 4-15, Unbekanntes -> '?')
void showUtf8(const char *in) {
  char t[17];
  uint8_t n = 0;
  const uint8_t *p = (const uint8_t *)in;
  while (*p && n < 16) {
    uint32_t cp = *p++;
    uint8_t more = cp >= 0xF0 ? 3 : cp >= 0xE0 ? 2 : cp >= 0xC0 ? 1 : 0;
    if (more) cp &= 0x3F >> more;
    while (more-- && (*p & 0xC0) == 0x80) cp = cp << 6 | (*p++ & 0x3F);
    if (cp == 0xFE0F || cp == 0x200D) continue;          // Varianten-Selektor, Verbinder
    char c = '?';
    if (cp >= 0x20 && cp < 0x7F) c = cp;
    else
      for (uint8_t i = 0; i < sizeof(GLYPH_MAP) / sizeof(GLYPH_MAP[0]); i++)
        if (GLYPH_MAP[i].cp == cp) { c = (char)(0x84 + GLYPH_MAP[i].g); break; }
    t[n++] = c;
  }
  t[n] = 0;
  showText(t);
}

void setBrightness(uint8_t b) {
  if (b > 7) return;
  brightness = b;
  for (uint8_t d = 0; d < 2; d++) dispWrite(d, 0x10, b);              // A4=1, A3=0: Control Word
  Serial.print(F("H ")); Serial.println(b);
}

void initDisplays() {
  const uint8_t ctrl[5] = {PIN_DRST, PIN_DWR, PIN_DRD, PIN_DCE[0], PIN_DCE[1]};
  for (uint8_t i = 0; i < 5; i++) { digitalWrite(ctrl[i], HIGH); pinMode(ctrl[i], OUTPUT); }
  digitalWrite(PIN_DFL, HIGH); pinMode(PIN_DFL, OUTPUT);   // /FL = 1: normale Register
  for (uint8_t i = 0; i < 5; i++) pinMode(DADDR[i], OUTPUT);
  for (uint8_t i = 0; i < 8; i++) pinMode(DDATA[i], OUTPUT);
  digitalWrite(PIN_DRST, LOW);
  delay(1);
  digitalWrite(PIN_DRST, HIGH);
  delay(10);
  memset(dispText, 0, sizeof(dispText));   // erzwingt das Schreiben aller Zeichen
  setBrightness(brightness);
  for (uint8_t d = 0; d < 2; d++)
    for (uint8_t g = 0; g < 12; g++) {
      dispWrite(d, 0x00, 4 + g);                        // UDC-Adresse
      for (uint8_t r = 0; r < 7; r++) dispWrite(d, 0x08 | r, GLYPH[g][r]);
    }
}

// Maennchen an Pixelposition x (0 = ganz links, Zelle = pitch Pixel), Lauf-Phase f.
// Die 2 Spaltpixel je Zelle sind unsichtbar: Das Maennchen verschwindet dort wie hinter einem Pfosten.
void drawMan(int x, uint8_t f) {
  int xs = x + pitch;
  int cell = xs / pitch - 1;
  uint8_t off = xs % pitch;
  // Neues Bild in das gerade NICHT angezeigte UDC-Paar schreiben, erst dann umschalten.
  // Sonst zeigt die alte Zelle kurz das neue Bild (Aufblitzen).
  udcBuf ^= 2;
  for (uint8_t d = 0; d < 2; d++) {
    for (uint8_t u = 0; u < 2; u++) {
      dispWrite(d, 0x00, udcBuf + u);            // A4=0, A3=0: UDC-Adresse
      for (uint8_t r = 0; r < 7; r++) {
        uint8_t row = MAN[f][r];
        if (manLeft)                               // spiegeln: Bit 4 <-> Bit 0, Bit 3 <-> Bit 1
          row = (row & 1) << 4 | (row & 2) << 2 | (row & 4) | (row & 8) >> 2 | (row & 16) >> 4;
        uint16_t v = (uint16_t)row << 11 >> off;  // 16 Spalten: Bit 15 = linke Spalte der Zelle
        dispWrite(d, 0x08 | r, u ? v >> (11 - pitch) & 31 : v >> 11 & 31);   // A4=0, A3=1: UDC-RAM
      }
    }
  }
  udcMain = udcBuf + (off < 3 ? 0 : 1);   // nur fuer die GUI
  char t[17];
  memset(t, ' ', 16);
  t[16] = 0;
  if (cell >= 0 && cell < 16) t[cell] = (char)(0x80 + udcBuf);
  if (cell + 1 >= 0 && cell + 1 < 16) t[cell + 1] = (char)(0x81 + udcBuf);
  showText(t);
}

void startSweep() {
  sweepOn = true;
  sweepManual = false;
  manLeft = false;
  sweepPhase = 0;
  sweepPos = -pitch;
  sweepNext = millis();
  setLed(SW_SWEEP, true);
}

void stopSweep() {
  if (!sweepOn) return;
  sweepOn = false;
  setLed(SW_SWEEP, false);
  showText("");
}

void runSweep() {
  if (!sweepOn || sweepManual || millis() < sweepNext) return;
  if (sweepPhase == 0) {
    sweepNext = millis() + MAN_MS;
    drawMan(sweepPos, (sweepPos + pitch) / 2 % 4);
    if (++sweepPos > 16 * pitch) { sweepPhase = 1; sweepPos = 0; }
  } else {
    sweepNext = millis() + SCROLL_MS;
    char t[17];
    int len = sizeof(SWEEP_TEXT) - 1;
    for (int i = 0; i < 16; i++) {
      int j = sweepPos + i - 16;                  // Text kommt von rechts herein
      t[i] = j >= 0 && j < len ? SWEEP_TEXT[j] : ' ';
    }
    t[16] = 0;
    showText(t);
    if (++sweepPos > len + 16) { sweepPhase = 0; sweepPos = -pitch; }
  }
}

void startClock() {
  clockOn = true;
  clockShown = 0xFFFFFFFF;
  setLed(SW_CLOCK, true);
}

void stopClock() {
  if (!clockOn) return;
  clockOn = false;
  setLed(SW_CLOCK, false);
  showText("");
}

void runClock() {
  if (!clockOn) return;
  if (!timeValid) { showText("UHR: KEINE ZEIT"); return; }
  unsigned long t = syncEpoch + (millis() - syncMillis) / 1000;
  if (t == clockShown) return;
  clockShown = t;
  // Tage seit 1970 -> Datum (Algorithmus "civil_from_days" von H. Hinnant)
  long z = t / 86400 + 719468;
  long era = z / 146097;
  unsigned doe = z - era * 146097;
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  unsigned day = doy - (153 * mp + 2) / 5 + 1;
  unsigned month = mp < 10 ? mp + 3 : mp - 9;
  unsigned long sec = t % 86400;
  char buf[17];
  // Uhrzeit links (DIS2), Datum rechtsbuendig (DIS1)
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u  %02u.%02u.",
           (unsigned)(sec / 3600), (unsigned)(sec / 60 % 60), (unsigned)(sec % 60), day, month);
  showText(buf);
}

// Maennchen um px Pixel bewegen (Drehrad), am Rand auf die andere Seite
void moveMan(long px) {
  if (!sweepManual) {
    sweepManual = true;
    if (sweepPhase != 0) { sweepPhase = 0; sweepPos = 0; }
  }
  manLeft = px < 0;
  int span = 17 * pitch;
  sweepPos = ((sweepPos + pitch + px) % span + span) % span - pitch;   // Bereich -pitch .. 16*pitch-1
  drawMan(sweepPos, (sweepPos + pitch) / 2 % 4);
}

void runEncoder() {
  noInterrupts();
  long n = encCount;
  interrupts();
  if (sweepOn && demo) {
    long px = (n - encBase) / ENC_MAN_DIV;
    if (px) {
      encBase += px * ENC_MAN_DIV;
      moveMan(px);
    }
  }
  long steps = (n - encBase) / ENC_DIV;
  if (steps && !sweepOn) {
    encBase += steps * ENC_DIV;
    if (demo) {
      stopSweep();
      stopClock();
      setLevel(barLevel + steps);
      showChainText();
    }
  }
  if (n != encReported && millis() >= encReportNext) {
    encReported = n;
    encReportNext = millis() + 50;
    Serial.print(F("E ")); Serial.println(n);
  }
}

// Tastenname links, Zusatz (z. B. "AN") rechtsbuendig
void showKey(uint8_t sw, const char *extra) {
  char t[17];
  snprintf(t, sizeof(t), "%-16s", KEY_NAME[sw]);
  uint8_t n = strlen(extra);
  if (n) memcpy(t + 16 - n, extra, n);
  showText(t);
}

void showChainText() {
  char t[17];
  snprintf(t, sizeof(t), "PEGEL   %2u / %2u", barLevel, BAR_LEN);
  showText(t);
}

// eine Stufe heller, nach 100 % wieder 13 %
void stepBrightness() {
  setBrightness(brightness == 0 || brightness > 6 ? 6 : brightness - 1);
  char t[17];
  snprintf(t, sizeof(t), "HELLIGKEIT %3s%%", BRIGHT_PCT[brightness]);
  showText(t);
}

void onPress(uint8_t sw) {
  if (!demo) return;
  if (sw == SW_CLOCK) {
    stopSweep();
    if (clockOn) stopClock(); else startClock();
    return;
  }
  stopClock();
  if (sw == SW_SWEEP) {
    if (sweepOn) stopSweep(); else startSweep();
    return;
  }
  if (sw == SW_SWEEP_AMPL) {
    pitch = pitch >= 10 ? 5 : pitch + 1;
    char t[17];
    snprintf(t, sizeof(t), "SPALT %u PIXEL", pitch - 5);
    if (!sweepOn) startSweep();
    showText(t);
    sweepPhase = 0;
    sweepPos = -pitch;
    sweepNext = millis() + 800;            // Text kurz stehen lassen
    return;
  }
  stopSweep();
  switch (sw) {
    case SW_ONAXIS:      setLevel(barLevel + 1); setLed(sw, true); showChainText(); return;
    case SW_2ND:         setLevel(barLevel - 1); setLed(sw, true); showChainText(); return;
    case SW_STDBY:
      routerBits = 0;
      channel = 0;
      setLevel(0);
      setLed(sw, true);
      showText("ROUTER AUS");
      return;
    case SW_TRANS_PDOWN: startAnim();            showKey(sw, "");                   return;
    case SW_CHAN_SELECT: nextChannel();                                             return;
    case SW_FINE:        stepBrightness();                                    return;
  }
  for (uint8_t i = 0; i < 5; i++)
    if (STATUS_KEYS[i][0] == sw) {
      bool on = statusMode(STATUS_KEYS[i][1]) == 0;
      setStatus(STATUS_KEYS[i][1], on ? 2 : 0);
      setLed(sw, on);
      showKey(sw, on ? "AN" : "AUS");
      return;
    }
  setLed(sw, !ledOf(sw));
  showKey(sw, ledOf(sw) ? "AN" : "AUS");
}

void runRepeat() {
  if (!repeatKey || millis() < repeatNext) return;
  if (repeatKey == SW_FINE) {
    repeatNext = millis() + REPEAT_FINE_MS;
    stepBrightness();
    return;
  }
  repeatNext = millis() + REPEAT_MS;
  setLevel(barLevel + (repeatKey == SW_ONAXIS ? 1 : -1));
  showChainText();
}

void onRelease(uint8_t sw) {
  if (sw == repeatKey) repeatKey = 0;
  if (demo && (sw == SW_ONAXIS || sw == SW_2ND || sw == SW_STDBY))
    setLed(sw, false);
}

// Ein kompletter Durchlauf: jede Zeile leuchten lassen und dabei die Tasten lesen.
void scan(bool *now) {
  memset(now, 0, NKEYS);
  bool blinkOff = (millis() / BLINK_MS) & 1;
  for (uint8_t r = 0; r < 7; r++) {
    uint8_t bits = ledRow[r];
    if (r == 6 && blinkOff) bits &= ~statusBlink;
    for (uint8_t c = 0; c < 6; c++) digitalWrite(BUS[c], bits >> c & 1);
    pinMode(ROWS[r], OUTPUT);
    digitalWrite(ROWS[r], HIGH);
    digitalWrite(PIN_OE, LOW);
    delayMicroseconds(30);
    if (r < 6)
      for (uint8_t c = 0; c < 6; c++)
        if (SW[r][c] && digitalRead(COLS[c]) == LOW) now[SW[r][c]] = true;
    delayMicroseconds(HOLD_US);
    digitalWrite(PIN_OE, HIGH);
    digitalWrite(ROWS[r], LOW);
    pinMode(ROWS[r], INPUT);   // Pull-down auf dem Board uebernimmt
  }
  digitalWrite(PIN_OE, LOW);   // zwischen den Durchlaeufen bleibt die Kette sichtbar
  now[4] = digitalRead(PIN_CHAN_SELECT) == LOW;
  now[5] = digitalRead(PIN_TRANS_PDOWN) == LOW;
}

void handleCommand() {
  if (cmd[0] == 'W') {               // Text unveraendert uebernehmen (auch Leerzeichen)
    stopSweep();
    stopClock();
    showUtf8(cmd.c_str() + 1);
    cmd = "";
    return;
  }
  cmd.trim();
  int eq = cmd.indexOf('=');
  if (cmd == "Q") {
    Serial.print('Q');
    for (uint8_t k = 1; k < NKEYS; k++)
      if (stable[k]) { Serial.print(F(" SW")); Serial.print(k); }
    Serial.println();
  } else if (cmd.length() >= 2 && cmd[0] == 'K') {
    uint8_t sw = cmd.substring(1).toInt();
    onPress(sw);
    onRelease(sw);
  } else if (cmd.length() >= 2 && cmd[0] == 'C') {
    setLevel(cmd.substring(1).toInt());
  } else if (cmd.length() >= 3 && (cmd[0] == 'L' || cmd[0] == 'S' || cmd[0] == 'R' || cmd[0] == 'B') && eq > 1) {
    int n = cmd.substring(1, eq).toInt();
    char v = cmd[cmd.length() - 1];
    if (cmd[0] == 'L') setLed(n, v == '1');
    else if (cmd[0] == 'R') setRouter(n, v == '1');
    else if (cmd[0] == 'B') setBar(n, v == '1');
    else setStatus(n, v - '0');
  } else if (cmd.length() == 2 && cmd[0] == 'T') {
    demo = cmd[1] == '1';
  } else if (cmd.length() >= 2 && cmd[0] == 'Z') {
    syncEpoch = strtoul(cmd.c_str() + 1, NULL, 10);
    syncMillis = millis();
    timeValid = true;
    clockShown = 0xFFFFFFFF;
  } else if (cmd.length() == 2 && cmd[0] == 'H') {
    setBrightness(cmd[1] - '0');
  } else if (cmd.length() == 2 && cmd[0] == 'A') {
    bool on = cmd[1] == '1';
    for (uint8_t sw = 1; sw < NKEYS; sw++) setLed(sw, on);
    for (uint8_t c = 1; c <= 6; c++) setStatus(c, on && c != 4 ? 1 : 0);
    routerBits = on ? (1UL << ROUTER_LEN) - 1 : 0;
    setLevel(on ? BAR_LEN : 0);
  }
  cmd = "";
}

void setup() {
  const uint8_t ctrl[4] = {PIN_OE, PIN_CLR, PIN_SRCLK, PIN_RCLK};
  for (uint8_t i = 0; i < 4; i++) { digitalWrite(ctrl[i], HIGH); pinMode(ctrl[i], OUTPUT); }
  digitalWrite(PIN_DLED, LOW);
  pinMode(PIN_DLED, OUTPUT);
  for (uint8_t c = 0; c < 6; c++) { digitalWrite(BUS[c], LOW); pinMode(BUS[c], OUTPUT); }
  for (uint8_t r = 0; r < 7; r++) pinMode(ROWS[r], INPUT);
  for (uint8_t c = 0; c < 6; c++) pinMode(COLS[c], INPUT_PULLUP);
  pinMode(PIN_CHAN_SELECT, INPUT_PULLUP);
  pinMode(PIN_TRANS_PDOWN, INPUT_PULLUP);
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("KEYS READY"));
  updateChain();
  initDisplays();
  digitalWrite(PIN_ENC_VCC, HIGH);       // Encoder-Versorgung (Notloesung)
  pinMode(PIN_ENC_VCC, OUTPUT);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  delay(20);
  encState = digitalRead(PIN_ENC_A) << 1 | digitalRead(PIN_ENC_B);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encISR, CHANGE);
  showText("  BSMS/2 READY");
}

void loop() {
  bool now[NKEYS];
  scan(now);
  for (uint8_t k = 1; k < NKEYS; k++) {
    if (now[k] == last[k] && now[k] != stable[k]) {   // 2 gleiche Scans = entprellt
      stable[k] = now[k];
      Serial.print(now[k] ? F("D SW") : F("U SW"));
      Serial.println(k);
      if (now[k]) {
        onPress(k);
        if (demo && (k == SW_ONAXIS || k == SW_2ND || k == SW_FINE)) { repeatKey = k; repeatNext = millis() + REPEAT_DELAY_MS; }
      } else {
        onRelease(k);
      }
    }
    last[k] = now[k];
  }
  runAnim();
  runRepeat();
  runSweep();
  runClock();
  runEncoder();
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { if (cmd.length()) handleCommand(); }
    else if (ch == 'Q' && cmd.length() == 0) { cmd = "Q"; handleCommand(); }   // 'Q' ohne Zeilenende wie bisher
    else if (cmd.length() < 70) cmd += ch;
  }
}
