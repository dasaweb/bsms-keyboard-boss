"""Sucht die Datenleitung (SER/DLED) der 74HC595-Kette per Halbierung.

Voraussetzung: Sketch arduino/led_probe ist auf dem Mega, Board versorgt, TP2 (P17) an VIN.
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\ser_finder.py COM8 [hoch]

Ohne Zusatz werden die in Ruhe HIGH liegenden Adern auf LOW gezogen (Lauf 2026-10-12: kein Treffer).
Mit "hoch" werden stattdessen die in Ruhe LOW liegenden Adern (P02, P04-P14, P52) auf HIGH getrieben
(falls SER z. B. über einen Inverter läuft).

Idee: SER liegt in Ruhe HIGH (jeder Takt schiebt eine 1 nach). Pro Runde wird eine Gruppe von
Adern auf ihren Gegenpegel getrieben, dann ein einziger Takt geschoben und übernommen. Ist SER in der Gruppe,
wird eine 0 geschoben, und das erste Bargraph-Segment bleibt AUS. Sonst geht es AN.
Eingabe: j = Segment leuchtet, n = Segment aus, q = beenden.
Ergebnis: logs/ser_finder_<zeit>.json
"""
import json
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

CTRL = {26, 55, 56, 57}    # /OE, /SRCLR, SRCLK, RCLK
IDLE_HIGH = [16, 18, 20, 22, 24, 25, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
             40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 53, 54, 58, 59]
IDLE_LOW = [2, 4, 6, 8, 10, 12, 14, 52]
LEVEL = "L"


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
            self.release()
            sys.exit(f"Befehl {text}: {answer or 'keine Antwort'} - abgebrochen, alles freigegeben.")

    def release(self):
        self.ser.write(b"R\n")
        self.ser.readline()

    def pulse(self, p):
        self.cmd(f"L{p}")
        self.cmd(f"F{p}")

    def shift_one(self, group):
        """Kette löschen, mit 'group' auf LOW einen Takt schieben, übernehmen."""
        self.release()
        self.cmd("L26")
        self.pulse(55)
        for p in group:
            self.cmd(f"{LEVEL}{p}")
        self.pulse(56)
        for p in group:
            self.cmd(f"F{p}")
        self.pulse(57)


def ask(text):
    while True:
        a = input(text + " [j/n/q] ").strip().lower()
        if a in ("j", "n", "q"):
            return a


def main():
    if len(sys.argv) < 2:
        sys.exit("Aufruf: python tools\\ser_finder.py COM8")
    global LEVEL
    high = len(sys.argv) > 2 and sys.argv[2].lower() == "hoch"
    LEVEL = "H" if high else "L"
    probe = Probe(sys.argv[1])
    logdir = Path(__file__).resolve().parent.parent / "logs"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"ser_finder_{datetime.now():%Y-%m-%d_%H%M%S}.json"
    log = []

    def test(group, label):
        probe.shift_one(group)
        a = ask(f">>> {label}: Leuchtet das ERSTE Segment des grünen Bargraphs LED25?")
        log.append({"gruppe": group, "antwort": a})
        logfile.write_text(json.dumps(log, indent=2), encoding="utf-8")
        return a

    print("Verbunden.\n")
    try:
        a = test([], "Kontrolle (keine Ader LOW, es sollte genau 1 Segment leuchten)")
        if a != "j":
            print("Kontrolle fehlgeschlagen - erwartet war 'j'. Abbruch.")
            return
        cands = IDLE_LOW if high else [p for p in IDLE_HIGH if p not in CTRL]
        word = "HIGH" if high else "LOW"
        a = test(cands, f"Alle {len(cands)} Kandidaten {word}")
        if a == "q":
            return
        if a == "j":
            print("\nSER ist keine der Kandidaten-Adern (fest verdrahtet oder anders angesteuert).")
            return
        rnd = 1
        while len(cands) > 1:
            half = cands[: len(cands) // 2]
            a = test(half, f"Runde {rnd}, {len(half)} Adern {word}")
            if a == "q":
                return
            cands = half if a == "n" else cands[len(cands) // 2:]
            rnd += 1
        p = cands[0]
        a = test([p], f"Bestätigung: nur P{p:02d} {word}")
        print(f"\n==> SER/DLED = P{p:02d}" if a == "n" else f"\nBestätigung für P{p:02d} fehlgeschlagen.")
        log.append({"ergebnis": f"P{p:02d}", "bestaetigt": a == "n"})
        logfile.write_text(json.dumps(log, indent=2), encoding="utf-8")
    finally:
        probe.release()
        print(f"Alles freigegeben. Log: {logfile}")
        input("Enter zum Schließen ...")


if __name__ == "__main__":
    main()
