// line_state – rein lesende Zustandsanalyse, Board VERSORGT
//
// Verdrahtung: P60 + TP3 (P11) an Mega-5V, JU1 (P01) + JU2 (P61) an GND,
// TP2 (P17, LED-Versorgung VS) offen. Diese Adern sind hier NICHT in der Liste.
// Der Sketch treibt NIE einen Pin (nur INPUT / INPUT_PULLUP), damit es keine
// Konflikte mit Ausgaengen auf dem Board geben kann.
//
// Ausgabe (115200 Baud):
//   Start: pro Ader Zustand ohne Pull-up / mit Pull-up und Klassifikation
//     HH = vom Board HIGH getrieben, LL = LOW getrieben (oder niederohmig nach GND),
//     ?H = offen bzw. Eingang auf dem Board (folgt dem Pull-up)
//     A-Pins zusaetzlich Spannung ohne Pull-up.
//   Danach (alle Pull-ups an): jede Pegelaenderung mit Zeitstempel, z. B. beim Tastendruck.
// Befehl 'm': Zustandstabelle erneut ausgeben.

struct Wire { uint8_t p; uint8_t pin; };

const Wire WIRES[] = {
  {2,23},
  {4,25},
  {6,27},
  {8,29},
  {10,31},
  {12,33},
  {14,35},
  {16,37},
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
};
const uint8_t N = sizeof(WIRES) / sizeof(WIRES[0]);

uint8_t stableLvl[N], lastLvl[N];

void printP(uint8_t i) {
  Serial.print('P');
  if (WIRES[i].p < 10) Serial.print('0');
  Serial.print(WIRES[i].p);
}

bool isAnalog(uint8_t pin) { return pin >= A0 && pin <= A15; }

void printTable() {
  uint8_t noPull[N], pull[N];
  int mv[N];
  for (uint8_t i = 0; i < N; i++) pinMode(WIRES[i].pin, INPUT);
  delay(20);
  for (uint8_t i = 0; i < N; i++) {
    noPull[i] = digitalRead(WIRES[i].pin);
    mv[i] = isAnalog(WIRES[i].pin) ? (long)analogRead(WIRES[i].pin) * 5000 / 1023 : -1;
  }
  for (uint8_t i = 0; i < N; i++) pinMode(WIRES[i].pin, INPUT_PULLUP);
  delay(20);
  for (uint8_t i = 0; i < N; i++) pull[i] = digitalRead(WIRES[i].pin);

  Serial.println(F("=== Zustand: Ader  ohnePU mitPU  Klasse  [mV ohne PU] ==="));
  for (uint8_t i = 0; i < N; i++) {
    printP(i);
    Serial.print(F("  ")); Serial.print(noPull[i] ? 'H' : 'L');
    Serial.print(F("  ")); Serial.print(pull[i] ? 'H' : 'L');
    Serial.print(F("  "));
    if (noPull[i] && pull[i]) Serial.print(F("HH getrieben HIGH"));
    else if (!noPull[i] && !pull[i]) Serial.print(F("LL getrieben LOW"));
    else if (!noPull[i] && pull[i]) Serial.print(F("?H offen/Eingang"));
    else Serial.print(F("H? seltsam"));
    if (mv[i] >= 0) { Serial.print(F("  ")); Serial.print(mv[i]); Serial.print(F(" mV")); }
    Serial.println();
  }
  Serial.println(F("=== Ende ==="));
  for (uint8_t i = 0; i < N; i++) stableLvl[i] = lastLvl[i] = pull[i];
}

void setup() {
  for (uint8_t i = 0; i < N; i++) pinMode(WIRES[i].pin, INPUT_PULLUP);
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("line_state BSMS/2 – Board versorgt, nur lesend"));
  printTable();
  Serial.println(F("Ueberwache Pegel (Pull-ups an). Jetzt Tasten druecken..."));
}

void loop() {
  if (Serial.available() && Serial.read() == 'm') printTable();
  for (uint8_t i = 0; i < N; i++) {
    uint8_t v = digitalRead(WIRES[i].pin);
    if (v == lastLvl[i] && v != stableLvl[i]) {   // zwei gleiche Lesungen in Folge
      stableLvl[i] = v;
      Serial.print(millis()); Serial.print(F(" ms  "));
      printP(i); Serial.println(v ? F(" -> HIGH") : F(" -> LOW"));
    }
    lastLvl[i] = v;
  }
  delay(2);
}
