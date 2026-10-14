// led_test – kompletter LED-Test des BSMS/2 Keyboard-Display-Boards (Board VERSORGT, TP2 an VIN)
//
// Schritte 0-40:  595-Kette (Router-LEDs, Bargraphs, TRANS. P-DOWN) mit k Einsen gefuellt,
//                 d. h. die ersten k Ausgaenge leuchten (SER ist fest auf +5 V verdrahtet).
// Schritte 41-82: Matrix-LEDs einzeln, Zeile r = 0-6 (P14 P12 P10 P08 P06 P04 P02), Spalte c = 0-5
//                 (Bus P46-P51); Schritt = 41 + r*6 + c. Die Kette ist dabei geloescht.
// Steuerung (115200 Baud, einzelne Zeichen): n = weiter, b = zurueck, a = Automatik an/aus,
//   0 = zum Anfang, m = zu den Matrix-LEDs. Ausgabe: "STEP <i>".
// Pins: P26 (D47) = /OE von 74HC245 und 595, P55 (D5) = /SRCLR, P56 (D11) = SRCLK, P57 (D6) = RCLK.

const uint8_t ROWS[7] = {35, 33, 31, 29, 27, 25, 23};   // P14 P12 P10 P08 P06 P04 P02
const uint8_t BUS[6]  = {A14, A7, A15, 2, 8, 3};        // P46 P47 P48 P49 P50 P51
const uint8_t PIN_OE = 47, PIN_CLR = 5, PIN_SRCLK = 11, PIN_RCLK = 6;
const uint8_t CHAIN = 40, LAST = 41 + 7 * 6 - 1;
const uint16_t AUTO_MS = 700;

int step = 0;
bool autoRun = false;
unsigned long lastStep = 0;

void pulse(uint8_t pin) {
  digitalWrite(pin, LOW);
  delayMicroseconds(5);
  digitalWrite(pin, HIGH);
  delayMicroseconds(5);
}

void fillChain(uint8_t k) {
  pulse(PIN_CLR);
  for (uint8_t i = 0; i < k; i++) pulse(PIN_SRCLK);
  pulse(PIN_RCLK);
}

void show() {
  for (uint8_t r = 0; r < 7; r++) { digitalWrite(ROWS[r], LOW); pinMode(ROWS[r], INPUT); }
  for (uint8_t c = 0; c < 6; c++) digitalWrite(BUS[c], LOW);
  if (step <= CHAIN) {
    fillChain(step);
  } else {
    fillChain(0);
    uint8_t idx = step - 41;
    digitalWrite(BUS[idx % 6], HIGH);
    pinMode(ROWS[idx / 6], OUTPUT);
    digitalWrite(ROWS[idx / 6], HIGH);
  }
  digitalWrite(PIN_OE, LOW);
  Serial.print(F("STEP "));
  Serial.println(step);
}

void setup() {
  const uint8_t ctrl[4] = {PIN_OE, PIN_CLR, PIN_SRCLK, PIN_RCLK};
  for (uint8_t i = 0; i < 4; i++) { digitalWrite(ctrl[i], HIGH); pinMode(ctrl[i], OUTPUT); }
  for (uint8_t c = 0; c < 6; c++) { digitalWrite(BUS[c], LOW); pinMode(BUS[c], OUTPUT); }
  for (uint8_t r = 0; r < 7; r++) pinMode(ROWS[r], INPUT);
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("LED TEST READY"));
  show();
}

void loop() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'n') step = step < LAST ? step + 1 : 0;
    else if (ch == 'b') step = step > 0 ? step - 1 : LAST;
    else if (ch == '0') step = 0;
    else if (ch == 'm') step = 41;
    else if (ch == 'a') autoRun = !autoRun;
    else return;
    lastStep = millis();
    show();
  }
  if (autoRun && millis() - lastStep >= AUTO_MS) {
    step = step < LAST ? step + 1 : 0;
    lastStep = millis();
    show();
  }
}
