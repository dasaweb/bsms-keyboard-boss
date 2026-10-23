// router_brute – sucht einen versteckten Weg, Router-LEDs einzeln zu schalten (Board VERSORGT, TP2 an VIN)
//
// Gesteuert von tools/router_brute.py. Befehle (115200 Baud, mit Zeilenende):
//   K<k>    Kette loeschen, k Einsen schieben, uebernehmen, /OE LOW (Ausgaenge an)
//   P<pos>  nur Kettenposition pos (1-40) an, 0 = alle aus; Daten ueber DLED = P60 (D14)
//   B<nr>[,<ms>[,<n>]]  Ader P<nr> n-mal wackeln (je <ms> LOW, <ms> HIGH; Vorgabe 700 ms, 3x),
//           danach freigeben bzw. bei den Kettensignalen auf ihren Ruhepegel zurueck
//   Z<ms>   Zufallstest der vier Kettensignale: je Schritt 1-8 schnelle Zufallszustaende,
//           dann <ms> halten. Ausgabe "Z <schritt> <zustaende hex>"; jedes Zeichen stoppt ("STOP").
//           Bit 0 = P28 (/OE), Bit 1 = P55 (/SRCLR), Bit 2 = P56 (SRCLK), Bit 3 = P57 (RCLK).
//   R       alles freigeben (INPUT_PULLUP)
// Antworten: "OK", "KONFLIKT L|H" (Board-Ausgang arbeitet dagegen, Pin sofort freigegeben), "?".
// P60 ist DLED (SER von IC10, Messung M3 2026-10-23) und haengt an D14. D14 wird erst mit K/P
// zum Ausgang, denn bei unversorgtem Board wuerde HIGH an SER das Board rueckspeisen.
// Wackeln treibt immer nur eine Ader. /CE und /WR der Displays sind nie gleichzeitig LOW,
// es wird also nichts in die Displays geschrieben.

struct Wire { uint8_t p; uint8_t pin; };

const Wire WIRES[] = {
  {4,23}, {6,25}, {8,27}, {10,29}, {12,31}, {14,33}, {16,35}, {18,37},
  {20,39}, {22,41}, {24,43}, {26,45}, {25,46}, {28,47}, {27,48}, {30,49},
  {29,50}, {32,51}, {31,52}, {33,A0}, {34,A8}, {35,A1}, {36,A9},
  {37,A2}, {38,A10}, {39,A3}, {40,A11}, {41,A4}, {42,A12}, {43,A5}, {44,A13},
  {45,A6}, {46,A14}, {47,A7}, {48,A15}, {49,2}, {50,8}, {51,3}, {52,9},
  {53,4}, {54,10}, {55,5}, {56,11}, {57,6}, {58,12}, {59,7},
};
const uint8_t N = sizeof(WIRES) / sizeof(WIRES[0]);

const uint8_t PIN_OE = 47, PIN_CLR = 5, PIN_SRCLK = 11, PIN_RCLK = 6;
const uint8_t PIN_DLED = 14;                                // P60
const uint8_t CTRL[4] = {PIN_OE, PIN_CLR, PIN_SRCLK, PIN_RCLK};

uint8_t ctrlIdle(uint8_t pin) { return pin == PIN_OE ? LOW : HIGH; }

bool isCtrl(uint8_t pin) {
  for (uint8_t i = 0; i < 4; i++) if (CTRL[i] == pin) return true;
  return false;
}

void releaseAll() {
  for (uint8_t i = 0; i < N; i++) pinMode(WIRES[i].pin, INPUT_PULLUP);
  pinMode(PIN_DLED, INPUT);
}

int8_t findWire(uint8_t p) {
  for (uint8_t i = 0; i < N; i++) if (WIRES[i].p == p) return i;
  return -1;
}

void pulse(uint8_t pin) {
  digitalWrite(pin, LOW);
  delayMicroseconds(5);
  digitalWrite(pin, HIGH);
  delayMicroseconds(5);
}

// Schiebt 40 Bits; das zuerst geschobene landet auf Position 40. on(pos) liefert das Bit.
void loadChain(bool (*on)(uint8_t, uint8_t), uint8_t arg) {
  for (uint8_t i = 1; i < 4; i++) { digitalWrite(CTRL[i], HIGH); pinMode(CTRL[i], OUTPUT); }
  pinMode(PIN_DLED, OUTPUT);
  for (uint8_t pos = 40; pos >= 1; pos--) {
    digitalWrite(PIN_DLED, on(pos, arg) ? HIGH : LOW);
    pulse(PIN_SRCLK);
  }
  digitalWrite(PIN_DLED, LOW);
  pulse(PIN_RCLK);
  digitalWrite(PIN_OE, LOW);
  pinMode(PIN_OE, OUTPUT);
}

bool prefix(uint8_t pos, uint8_t k) { return pos <= k; }
bool single(uint8_t pos, uint8_t p) { return pos == p; }

void fillChain(uint8_t k) { loadChain(prefix, k); }

// Treibt den Pin und liest zurueck; false = Konflikt (Pin ist dann schon freigegeben).
bool drive(uint8_t pin, uint8_t level) {
  digitalWrite(pin, level);
  pinMode(pin, OUTPUT);
  delayMicroseconds(200);
  if (digitalRead(pin) == level) return true;
  pinMode(pin, INPUT_PULLUP);
  return false;
}

void blink(uint8_t pin, uint16_t ms, uint8_t count) {
  for (uint8_t n = 0; n < count; n++) {
    if (!drive(pin, LOW))  { Serial.println(F("KONFLIKT L")); return; }
    delay(ms);
    if (!drive(pin, HIGH)) { Serial.println(F("KONFLIKT H")); return; }
    delay(ms);
  }
  if (isCtrl(pin)) digitalWrite(pin, ctrlIdle(pin));
  else pinMode(pin, INPUT_PULLUP);
  Serial.println(F("OK"));
}

void setCtrl(uint8_t state) {
  for (uint8_t i = 0; i < 4; i++) digitalWrite(CTRL[i], (state >> i) & 1);
}

void fuzz(uint16_t holdMs) {
  for (uint8_t i = 0; i < 4; i++) pinMode(CTRL[i], OUTPUT);
  randomSeed(micros());
  char hex[] = "0123456789ABCDEF";
  for (uint32_t step = 0; !Serial.available(); step++) {
    uint8_t n = random(1, 9);
    char seq[9];
    for (uint8_t i = 0; i < n; i++) {
      uint8_t s = random(16);
      setCtrl(s);
      delayMicroseconds(5);
      seq[i] = hex[s];
    }
    seq[n] = 0;
    Serial.print(F("Z "));
    Serial.print(step);
    Serial.print(' ');
    Serial.println(seq);
    delay(holdMs);
  }
  while (Serial.available()) Serial.read();
  Serial.println(F("STOP"));
}

void setup() {
  releaseAll();
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("ROUTER BRUTE READY"));
}

void loop() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "R") { releaseAll(); Serial.println(F("OK")); return; }
  if (cmd.length() < 2) { Serial.println('?'); return; }
  long arg = cmd.substring(1).toInt();
  if (cmd[0] == 'K' && arg >= 0 && arg <= 40) { fillChain(arg); Serial.println(F("OK")); return; }
  if (cmd[0] == 'P' && arg >= 0 && arg <= 40) { loadChain(single, arg); Serial.println(F("OK")); return; }
  if (cmd[0] == 'Z' && arg > 0) { fuzz(arg); return; }
  if (cmd[0] == 'B') {
    int8_t i = findWire(arg);
    if (i < 0) { Serial.println('?'); return; }
    int c1 = cmd.indexOf(','), c2 = cmd.indexOf(',', c1 + 1);
    uint16_t ms = c1 > 0 ? cmd.substring(c1 + 1).toInt() : 700;
    uint8_t count = c2 > 0 ? cmd.substring(c2 + 1).toInt() : 3;
    blink(WIRES[i].pin, ms ? ms : 700, count ? count : 3);
    return;
  }
  Serial.println('?');
}
