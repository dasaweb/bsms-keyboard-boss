// led_probe – einzelne Adern statisch treiben, um die LED-Ansteuerung zu finden (Board VERSORGT)
//
// Gesteuert von tools/led_finder.py. Befehle (115200 Baud, mit Zeilenende):
//   L<nr>  Ader P<nr> auf LOW treiben      H<nr>  Ader P<nr> auf HIGH treiben
//   F<nr>  Ader P<nr> freigeben              R      alle Adern freigeben (INPUT_PULLUP)
// Mehrere L/H-Befehle hintereinander treiben mehrere Adern gleichzeitig.
// Antworten: "OK", "KONFLIKT" (die Ader wird auf dem Board aktiv dagegen gehalten,
// sie wird sofort wieder freigegeben) oder "?" (Ader unbekannt).
// Sicherheit: Nach dem Treiben wird der eigene Pegel zurueckgelesen. Weicht er ab,
// arbeitet ein Board-Ausgang dagegen und der Pin wird sofort hochohmig geschaltet.

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

void releaseAll() {
  for (uint8_t i = 0; i < N; i++) pinMode(WIRES[i].pin, INPUT_PULLUP);
}

int8_t findWire(uint8_t p) {
  for (uint8_t i = 0; i < N; i++) if (WIRES[i].p == p) return i;
  return -1;
}

void setup() {
  releaseAll();
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("LED PROBE READY"));
}

void loop() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "R") { releaseAll(); Serial.println(F("OK")); return; }
  if (cmd.length() < 2 || (cmd[0] != 'L' && cmd[0] != 'H' && cmd[0] != 'F')) { Serial.println('?'); return; }
  int8_t i = findWire(cmd.substring(1).toInt());
  if (i < 0) { Serial.println('?'); return; }
  if (cmd[0] == 'F') { pinMode(WIRES[i].pin, INPUT_PULLUP); Serial.println(F("OK")); return; }
  uint8_t level = cmd[0] == 'H' ? HIGH : LOW;
  digitalWrite(WIRES[i].pin, level);
  pinMode(WIRES[i].pin, OUTPUT);
  delayMicroseconds(200);
  if (digitalRead(WIRES[i].pin) != level) {
    pinMode(WIRES[i].pin, INPUT_PULLUP);
    Serial.println(F("KONFLIKT"));
  } else {
    Serial.println(F("OK"));
  }
}
