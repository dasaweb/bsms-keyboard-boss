"""Interaktiver Test der 74HC595-Kette (Router-LEDs LED1-19, Bargraphs, ggf. TRANS. P-DOWN).

Voraussetzung: Sketch arduino/led_probe ist auf dem Mega, Board versorgt, TP2 (P17) an VIN.
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\router_finder.py COM8

Bekannt (pin_tracer 2026-10-12): SRCLK = P56, RCLK = P57, /OE = P26, /SRCLR = P55.
Der Dateneingang SER von IC10 ist noch unbekannt.
Ablauf:
  1. P26 LOW (Ausgänge frei): Was leuchtet?
  2. Kette löschen (P55-Puls) und übernehmen (P57-Puls): Ist alles aus?
  3. Je Enter ein Schiebetakt (P56) plus Übernahme (P57): Was hat sich geändert?
     Kommt bei jedem Takt eine LED dazu, liegt SER auf HIGH, und wir lernen die Reihenfolge.
Eingabe: Beschreibung oder Enter = nichts, q = beenden.
Ergebnis: logs/router_finder_<zeit>.json (wird nach jeder Antwort gespeichert).
"""
import json
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

STEPS = 40   # 5 x 74HC595


class Probe:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=2)
        deadline = time.time() + 6
        while time.time() < deadline:
            if b"LED PROBE READY" in self.ser.readline():
                return
        sys.exit("Kein READY vom Mega - ist der Sketch led_probe hochgeladen?")

    def cmd(self, text):
        self.ser.write((text + "\n").encode())
        answer = self.ser.readline().decode(errors="replace").strip()
        if answer != "OK":
            self.ser.write(b"R\n")
            self.ser.readline()
            sys.exit(f"Befehl {text}: {answer or 'keine Antwort'} - abgebrochen, alles freigegeben.")

    def pulse(self, p):
        """LOW-Puls; beim Freigeben zieht der Board-Pull-up wieder auf HIGH (steigende Flanke)."""
        self.cmd(f"L{p}")
        self.cmd(f"F{p}")


def main():
    if len(sys.argv) < 2:
        sys.exit("Aufruf: python tools\\router_finder.py COM8")
    probe = Probe(sys.argv[1])
    probe.cmd("R")

    logdir = Path(__file__).resolve().parent.parent / "logs"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"router_finder_{datetime.now():%Y-%m-%d_%H%M%S}.json"
    results = {}

    def ask(key, question):
        ans = input(question + " ").strip()
        results[key] = ans
        logfile.write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding="utf-8")
        return ans.lower() != "q"

    print("Verbunden.\n")
    try:
        probe.cmd("L26")
        if not ask("freigegeben", ">>> P26 ist LOW. Was leuchtet oben (LED1-19, Bargraphs, TRANS. P-DOWN)?"):
            return
        probe.pulse(55)
        probe.pulse(57)
        if not ask("geloescht", ">>> Kette gelöscht und übernommen. Ist jetzt oben alles aus? Was leuchtet noch?"):
            return
        print("\nJetzt kommt je Enter ein Schiebetakt. Beschreibe, welche LED neu an (oder aus) ist.\n")
        for k in range(1, STEPS + 1):
            probe.pulse(56)
            probe.pulse(57)
            if not ask(f"takt_{k:02d}", f"[Takt {k}/{STEPS}] Was hat sich geändert? (Enter = nichts, q = Ende)"):
                return
    finally:
        probe.ser.write(b"R\n")
        probe.ser.readline()
        print(f"\nFertig, alles freigegeben. Ergebnis gespeichert in {logfile}")
        input("Enter zum Schließen ...")


if __name__ == "__main__":
    main()
