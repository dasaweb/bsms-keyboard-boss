// matrix_probe – Brute-Force-Suche nach den Tasten-Zeilenleitungen (Board VERSORGT)
//
// Verdrahtung wie line_state: P60 + TP3 (P11) an 5V, JU1 (P01) + JU2 (P61) an GND,
// TP2 (P17) haengt an D38, wird aber nie benutzt.
// Gesteuert von tools/matrix_finder.py. Protokoll (115200 Baud):
//   Start:  "READY <n>" und pro Ader "W <idx> <P-Nummer> <flags>"
//   'S'  -> ein Scan: "S <basis-bitmap>", dann pro treibbarer Ader
//           "D <idx> <L|H> <bitmap waehrend des Treibens>", Abschluss "E".
//           Bitmaps: Bit i = Pegel von WIRES[i] (1 = HIGH), hex, 16 Stellen.
// Treiben nur kurz (50 us) und nur fuer Adern mit Flag, damit ein evtl. Board-
// Ausgang nie laenger gegen den Mega arbeitet.
// LOW: alle im Ruhezustand HIGH liegenden Adern. HIGH: die im Ruhezustand LOW
// liegenden Adern P02, P04-P14 (evtl. 74HC05-Eingaenge mit Pull-down) und P52. Alle anderen Pins: INPUT_PULLUP.

#define CAN_LOW  1
#define CAN_HIGH 2

struct Wire { uint8_t p; uint8_t pin; uint8_t flags; };

const Wire WIRES[] = {
  {2,23,CAN_HIGH},
  {4,25,CAN_HIGH},
  {6,27,CAN_HIGH},
  {8,29,CAN_HIGH},
  {10,31,CAN_HIGH},
  {12,33,CAN_HIGH},
  {14,35,CAN_HIGH},
  {16,37,CAN_LOW},
  {18,39,CAN_LOW},
  {20,41,CAN_LOW},
  {22,43,CAN_LOW},
  {24,45,CAN_LOW},
  {25,46,CAN_LOW},
  {26,47,CAN_LOW},
  {27,48,CAN_LOW},
  {28,49,CAN_LOW},
  {29,50,CAN_LOW},
  {30,51,CAN_LOW},
  {31,52,CAN_LOW},
  {32,53,CAN_LOW},
  {33,A0,CAN_LOW},
  {34,A8,CAN_LOW},
  {35,A1,CAN_LOW},
  {36,A9,CAN_LOW},
  {37,A2,CAN_LOW},
  {38,A10,CAN_LOW},
  {39,A3,CAN_LOW},
  {40,A11,CAN_LOW},
  {41,A4,CAN_LOW},
  {42,A12,CAN_LOW},
  {43,A5,CAN_LOW},
  {44,A13,CAN_LOW},
  {45,A6,CAN_LOW},
  {46,A14,CAN_LOW},
  {47,A7,CAN_LOW},
  {48,A15,CAN_LOW},
  {49,2,CAN_LOW},
  {50,8,CAN_LOW},
  {51,3,CAN_LOW},
  {52,9,CAN_HIGH},
  {53,4,CAN_LOW},
  {54,10,CAN_LOW},
  {55,5,CAN_LOW},
  {56,11,CAN_LOW},
  {57,6,CAN_LOW},
  {58,12,0},
  {59,7,0},
};
const uint8_t N = sizeof(WIRES) / sizeof(WIRES[0]);

uint64_t readAll() {
  uint64_t m = 0;
  for (uint8_t i = 0; i < N; i++)
    if (digitalRead(WIRES[i].pin)) m |= (uint64_t)1 << i;
  return m;
}

void printHex64(uint64_t v) {
  for (int8_t s = 60; s >= 0; s -= 4) Serial.print((uint8_t)((v >> s) & 0xF), HEX);
}

void driveAndReport(uint8_t i, uint8_t level) {
  digitalWrite(WIRES[i].pin, level);       // erst Pegel setzen (bei LOW: Pull-up aus)
  pinMode(WIRES[i].pin, OUTPUT);
  delayMicroseconds(50);
  uint64_t m = readAll();
  pinMode(WIRES[i].pin, INPUT_PULLUP);
  delayMicroseconds(300);
  Serial.print(F("D ")); Serial.print(i); Serial.print(level ? F(" H ") : F(" L "));
  printHex64(m); Serial.println();
}

void scan() {
  Serial.print(F("S ")); printHex64(readAll()); Serial.println();
  for (uint8_t i = 0; i < N; i++) {
    if (WIRES[i].flags & CAN_LOW)  driveAndReport(i, LOW);
    if (WIRES[i].flags & CAN_HIGH) driveAndReport(i, HIGH);
  }
  Serial.println('E');
}

void setup() {
  for (uint8_t i = 0; i < N; i++) pinMode(WIRES[i].pin, INPUT_PULLUP);
  Serial.begin(115200);
  while (!Serial) {}
  delay(50);
  Serial.print(F("READY ")); Serial.println(N);
  for (uint8_t i = 0; i < N; i++) {
    Serial.print(F("W ")); Serial.print(i); Serial.print(' ');
    Serial.print(WIRES[i].p); Serial.print(' '); Serial.println(WIRES[i].flags);
  }
}

void loop() {
  if (Serial.available() && Serial.read() == 'S') scan();
}
