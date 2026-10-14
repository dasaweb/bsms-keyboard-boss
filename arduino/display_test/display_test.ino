// display_test: erster Schreibtest der beiden HDSP-2112 (DIS1, DIS2).
// Pinbelegung siehe PINOUT.md, Abschnitt "Displays HDSP-2112" (pin_tracer 2026-10-13).
// Versorgung: Board versorgt (P60/TP3 an 5V), TP2 (P17) wird nicht gebraucht.
//
// Ablauf beim Start: Reset (/RST), Control Word (Helligkeit), dann "DIS1 ..." / "DIS2 ..." schreiben.
// Befehle (Zeilenende \n):
//   1<text>  bis 8 Zeichen auf DIS1      2<text>  bis 8 Zeichen auf DIS2
//   H<0-7>   Helligkeit (0 = 100 %, 3 = 40 %, 6 = 13 %, 7 = aus), für beide
//   X        Selbsttest beider Displays (alle Punkte an, ca. 5 s) - Strom!
//   R        Reset und Starttext
//   ?        Status

const uint8_t PIN_RST = A3;   // P39
const uint8_t PIN_FL  = 41;   // P20
const uint8_t PIN_WR  = 10;   // P54
const uint8_t PIN_RD  = 4;    // P53
const uint8_t PIN_CE[2] = {37, 39};  // P16 = DIS1, P18 = DIS2
const uint8_t ADDR[5] = {43, 46, 48, 50, 52};               // A0-A4 = P22, P25, P27, P29, P31
const uint8_t DATA[8] = {A10, A2, A9, A1, A8, A0, 51, 49};  // D0-D7 = P38, P37, P36, P35, P34, P33, P30, P28

uint8_t brightness = 3;   // 40 %, schont das Netzteil

void writeReg(uint8_t disp, bool fl, uint8_t addr, uint8_t data) {
  digitalWrite(PIN_FL, fl ? HIGH : LOW);
  for (uint8_t i = 0; i < 5; i++) digitalWrite(ADDR[i], (addr >> i) & 1);
  for (uint8_t i = 0; i < 8; i++) digitalWrite(DATA[i], (data >> i) & 1);
  digitalWrite(PIN_CE[disp], LOW);
  delayMicroseconds(1);
  digitalWrite(PIN_WR, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_WR, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_CE[disp], HIGH);
}

void control(uint8_t disp, uint8_t word) { writeReg(disp, true, 0x10, word); }   // A4=1, A3=0

void text(uint8_t disp, const char *s) {
  for (uint8_t pos = 0; pos < 8; pos++) {
    char c = *s ? *s++ : ' ';
    writeReg(disp, true, 0x18 | pos, (uint8_t)c);   // A4=1, A3=1: Zeichen-RAM
  }
}

void resetDisplays() {
  digitalWrite(PIN_RST, LOW);
  delay(1);
  digitalWrite(PIN_RST, HIGH);
  delay(10);
  for (uint8_t d = 0; d < 2; d++) control(d, brightness);
  text(0, "DIS1 ok");
  text(1, "DIS2 ok");
}

void setup() {
  Serial.begin(115200);
  // Ruhepegel: alle Steuerleitungen inaktiv HIGH, dann erst Ausgang
  const uint8_t ctrl[6] = {PIN_RST, PIN_FL, PIN_WR, PIN_RD, PIN_CE[0], PIN_CE[1]};
  for (uint8_t i = 0; i < 6; i++) {
    digitalWrite(ctrl[i], HIGH);
    pinMode(ctrl[i], OUTPUT);
  }
  for (uint8_t i = 0; i < 5; i++) pinMode(ADDR[i], OUTPUT);
  for (uint8_t i = 0; i < 8; i++) pinMode(DATA[i], OUTPUT);
  resetDisplays();
  Serial.println(F("DISPLAY TEST READY"));
}

void loop() {
  static char buf[20];
  static uint8_t n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (n < sizeof(buf) - 1) buf[n++] = c;
      continue;
    }
    buf[n] = 0;
    n = 0;
    if (buf[0] == '1' || buf[0] == '2') {
      text(buf[0] - '1', buf + 1);
      Serial.println(F("OK"));
    } else if (buf[0] == 'H' && buf[1] >= '0' && buf[1] <= '7') {
      brightness = buf[1] - '0';
      for (uint8_t d = 0; d < 2; d++) control(d, brightness);
      Serial.println(F("OK"));
    } else if (buf[0] == 'X') {
      for (uint8_t d = 0; d < 2; d++) control(d, 0x40 | brightness);
      Serial.println(F("OK Selbsttest"));
    } else if (buf[0] == 'R') {
      resetDisplays();
      Serial.println(F("OK"));
    } else if (buf[0] == '?') {
      Serial.print(F("Helligkeit "));
      Serial.println(brightness);
    } else if (buf[0]) {
      Serial.println(F("? unbekannt"));
    }
  }
}
