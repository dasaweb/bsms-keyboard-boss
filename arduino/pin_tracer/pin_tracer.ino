// pin_tracer – findet, welche Kabelader an einem beliebigen Board-Punkt ankommt (Board VERSORGT)
//
// Ein Jumperkabel an D13 dient als Messspitze. Der Nutzer haelt es an einen IC-Pin.
// Gesteuert von tools/pin_tracer.py. Befehl "T" (mit Zeilenende):
//   Jede Ader wird nacheinander kurz auf den Gegenpegel ihres Ruhezustands getrieben
//   (Ruhe HIGH -> LOW, Ruhe LOW -> HIGH), dabei wird die Spitze gelesen.
//   Antwort: "T <ruhepegel der spitze> <treffer...> E", Treffer als P<nr>+ (folgt) bzw. P<nr>- (invertiert).
// Basis: "L<nr>" / "H<nr>" haelt eine Ader dauerhaft auf LOW/HIGH (Antwort OK/KONFLIKT/?),
//   "R" gibt alle frei. Gehaltene Adern werden beim Trace uebersprungen.
// Sicherheit: Rueckleseprüfung wie in led_probe. Die Spitze ist immer INPUT_PULLUP, sie treibt nie.
// Die Spitze NIE an Punkte ueber 5 V halten (UDN2982A Pin 9-18, ULN2804A Pin 10-18)!

struct Wire { uint8_t p; uint8_t pin; };

const uint8_t PROBE = 13;

const Wire WIRES[] = {
  {4,23},
  {6,25},
  {8,27},
  {10,29},
  {12,31},
  {14,33},
  {16,35},
  {18,37},
  {20,39},
  {22,41},
  {24,43},
  {26,45},
  {25,46},
  {28,47},
  {27,48},
  {30,49},
  {29,50},
  {32,51},
  {31,52},
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
const uint8_t REPEATS = 5;
bool held[sizeof(WIRES) / sizeof(WIRES[0])];

int8_t findWire(uint8_t p) {
  for (uint8_t i = 0; i < N; i++) if (WIRES[i].p == p) return i;
  return -1;
}

void releaseAll() {
  for (uint8_t i = 0; i < N; i++) { pinMode(WIRES[i].pin, INPUT_PULLUP); held[i] = false; }
}

// 1 = Spitze folgt, 2 = invertiert, 0 = keine Wirkung/inkonsistent, 3 = Konflikt
uint8_t testWire(uint8_t i) {
  uint8_t pin = WIRES[i].pin;
  uint8_t idle = digitalRead(pin);
  uint8_t level = !idle;
  uint8_t follow = 0, invert = 0;
  for (uint8_t r = 0; r < REPEATS; r++) {
    uint8_t before = digitalRead(PROBE);
    digitalWrite(pin, level);
    pinMode(pin, OUTPUT);
    delayMicroseconds(40);
    if (digitalRead(pin) != level) { pinMode(pin, INPUT_PULLUP); return 3; }
    uint8_t during = digitalRead(PROBE);
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(200);
    uint8_t after = digitalRead(PROBE);
    if (before == after && during != before) {
      if (during == level) follow++; else invert++;
    }
  }
  if (follow == REPEATS) return 1;
  if (invert == REPEATS) return 2;
  return 0;
}

void setup() {
  pinMode(PROBE, INPUT_PULLUP);
  releaseAll();
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("TRACER READY"));
}

void loop() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "R") { releaseAll(); Serial.println(F("OK")); return; }
  if (cmd.length() >= 2 && (cmd[0] == 'L' || cmd[0] == 'H')) {
    int8_t i = findWire(cmd.substring(1).toInt());
    if (i < 0) { Serial.println('?'); return; }
    uint8_t level = cmd[0] == 'H' ? HIGH : LOW;
    digitalWrite(WIRES[i].pin, level);
    pinMode(WIRES[i].pin, OUTPUT);
    delayMicroseconds(200);
    if (digitalRead(WIRES[i].pin) != level) { pinMode(WIRES[i].pin, INPUT_PULLUP); Serial.println(F("KONFLIKT")); return; }
    held[i] = true;
    Serial.println(F("OK"));
    return;
  }
  if (cmd != "T") { Serial.println('?'); return; }
  Serial.print(F("T "));
  Serial.print(digitalRead(PROBE) ? 'H' : 'L');
  for (uint8_t i = 0; i < N; i++) {
    if (held[i]) continue;
    uint8_t res = testWire(i);
    if (res == 0) continue;
    Serial.print(F(" P"));
    if (WIRES[i].p < 10) Serial.print('0');
    Serial.print(WIRES[i].p);
    Serial.print(res == 1 ? '+' : res == 2 ? '-' : '!');
  }
  Serial.println(F(" E"));
}
