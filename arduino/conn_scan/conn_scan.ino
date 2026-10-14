// conn_scan – passiver Durchgangsscanner fuer das BSMS/2 Keyboard-Display-Board
//
// Board ist NICHT versorgt. Nur JU1 (P01/03/05/07/09) haengt an Mega-GND.
// Pins werden nie HIGH getrieben: immer genau ein Pin OUTPUT LOW, alle anderen
// INPUT_PULLUP. Damit wird nichts rueckgespeist und es gibt keine Kurzschluesse.
//
// Ausgabe (115200 Baud):
//   - beim Start: Adern, die schon ohne Treiben LOW sind (Verbindung nach GND/JU1),
//     und alle Gruppen untereinander verbundener Adern
//   - danach laufend: Aenderungen, z. B. "+ P12 -> P40" beim Druecken einer Taste,
//     "- P12 -> P40" beim Loslassen. "A -> B" heisst: A auf LOW zieht B auf LOW.
// Befehl ueber Serial: 'm' = komplette Karte erneut ausgeben.

struct Wire { uint8_t p; uint8_t pin; };

// Zuordnung laut PINOUT.md. JU1 fehlt (liegt an GND). Von den kurzgeschlossenen
// Gruppen ist nur je eine Ader angeschlossen: TP3 -> P11, TP2 -> P17, JU2 -> P61.
const Wire WIRES[] = {
  {2,23},
  {4,25},
  {6,27},
  {8,29},
  {10,31},
  {11,32},
  {12,33},
  {14,35},
  {16,37},
  {17,38},
  {18,39},
  {20,41},
  {22,43},
  {24,45},
  {25,46},
  {26,47},
  {27,48},
  {28,49},
  {29,50},
  {30,51},
  {31,52},
  {32,53},
  {33,A0},
  {34,A8},
  {35,A1},
  {36,A9},
  {37,A2},
  {38,A10},
  {39,A3},
  {40,A11},
  {41,A4},
  {42,A12},
  {43,A5},
  {44,A13},
  {45,A6},
  {46,A14},
  {47,A7},
  {48,A15},
  {49,2},
  {50,8},
  {51,3},
  {52,9},
  {53,4},
  {54,10},
  {55,5},
  {56,11},
  {57,6},
  {58,12},
  {59,7},
  {60,14},
  {61,15},
};
const uint8_t N = sizeof(WIRES) / sizeof(WIRES[0]);
const uint8_t BYTES = (N + 7) / 8;

uint8_t cur[N][BYTES], prev[N][BYTES], stable[N][BYTES];
uint8_t grounded[BYTES];

inline bool getBit(const uint8_t *row, uint8_t j) { return row[j >> 3] & (1 << (j & 7)); }
inline void setBit(uint8_t *row, uint8_t j) { row[j >> 3] |= (1 << (j & 7)); }

void printP(uint8_t i) {
  Serial.print('P');
  if (WIRES[i].p < 10) Serial.print('0');
  Serial.print(WIRES[i].p);
}

void allInputs() {
  for (uint8_t i = 0; i < N; i++) pinMode(WIRES[i].pin, INPUT_PULLUP);
}

void scan(uint8_t m[N][BYTES]) {
  memset(m, 0, N * BYTES);
  for (uint8_t i = 0; i < N; i++) {
    if (getBit(grounded, i)) continue;
    pinMode(WIRES[i].pin, OUTPUT);
    digitalWrite(WIRES[i].pin, LOW);
    delayMicroseconds(100);
    for (uint8_t j = 0; j < N; j++) {
      if (j == i || getBit(grounded, j)) continue;
      if (digitalRead(WIRES[j].pin) == LOW) setBit(m[i], j);
    }
    pinMode(WIRES[i].pin, INPUT_PULLUP);
    delayMicroseconds(100);   // Leitung wieder hochziehen lassen
  }
}

void findGrounded() {
  memset(grounded, 0, BYTES);
  allInputs();
  delay(10);
  for (uint8_t i = 0; i < N; i++)
    if (digitalRead(WIRES[i].pin) == LOW) setBit(grounded, i);
}

void printMap() {
  Serial.println(F("=== Adern LOW ohne Treiben (Verbindung zu GND/JU1) ==="));
  bool any = false;
  for (uint8_t i = 0; i < N; i++)
    if (getBit(grounded, i)) { printP(i); Serial.print(' '); any = true; }
  Serial.println(any ? "" : "(keine)");

  Serial.println(F("=== Verbindungen (A -> B: A LOW zieht B LOW; <-> = beidseitig) ==="));
  any = false;
  for (uint8_t i = 0; i < N; i++)
    for (uint8_t j = 0; j < N; j++) {
      if (!getBit(stable[i], j)) continue;
      bool both = getBit(stable[j], i);
      if (both && j < i) continue;
      printP(i); Serial.print(both ? F(" <-> ") : F(" -> ")); printP(j); Serial.println();
      any = true;
    }
  if (!any) Serial.println(F("(keine)"));
  Serial.println(F("=== Ende ==="));
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("conn_scan BSMS/2 – Board unversorgt, nur JU1 an GND!"));
  findGrounded();
  scan(stable);
  memcpy(prev, stable, sizeof(stable));
  printMap();
  Serial.println(F("Jetzt Tasten druecken..."));
}

void loop() {
  if (Serial.available() && Serial.read() == 'm') printMap();

  scan(cur);
  // Entprellen: Aenderung erst uebernehmen, wenn zwei Scans uebereinstimmen
  for (uint8_t i = 0; i < N; i++)
    for (uint8_t j = 0; j < N; j++) {
      bool c = getBit(cur[i], j), p = getBit(prev[i], j), s = getBit(stable[i], j);
      if (c == p && c != s) {
        if (c) stable[i][j >> 3] |= (1 << (j & 7));
        else   stable[i][j >> 3] &= ~(1 << (j & 7));
        Serial.print(c ? F("+ ") : F("- "));
        printP(i); Serial.print(F(" -> ")); printP(j); Serial.println();
      }
    }
  memcpy(prev, cur, sizeof(cur));
}
