"""Interaktive Leitungsverfolgung: Welche Kabelader kommt an welchem IC-Pin an?

Voraussetzung: Sketch arduino/pin_tracer ist auf dem Mega, Board versorgt (P60/P11 an 5V),
ein Jumperkabel steckt in D13 und dient als Messspitze.
TP2 (P17) NICHT an VIN, sondern abgezogen lassen (dann liegt nirgends mehr als 5 V an).
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\pin_tracer.py COM8 [led | router | eigene Punkte ...]

Vorgaben: "led" = IC1/IC3 Pin 1-8 (Standard), "router" = 74HC595 IC10-IC14 Pin 10-14
(/SRCLR, SRCLK, RCLK, /OE, SER), "dis1"/"dis2" = alle 28 Lötpunkte eines HDSP-2112 nach Lage
auf der Rückseite (oben01-14, unten01-14, je von links). Statt einer Vorgabe können auch Punktnamen folgen.
Argumente mit "+" werden als Basis dauerhaft gehalten, z. B. +L26 = P26 LOW
(gibt den 74HC245 und die 595-Ausgänge frei).

Ablauf: Das Programm nennt einen IC-Pin. Messspitze (D13-Kabel) an den Pin halten, Enter drücken,
halten, bis das Ergebnis da ist (ca. 0,1 s).
Eingabe: Enter = vorgeschlagenen Pin messen, eigener Name (z. B. "DIS1.13") = diesen Punkt messen,
         w = letzte Messung wiederholen, s = überspringen, z = zurück, q = beenden.
Ergebnis: logs/pin_tracer_<zeit>.json (wird nach jeder Messung gespeichert).
"""
import json
import re
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

PRESETS = {
    "led": [f"IC1.{n}" for n in range(1, 9)] + [f"IC3.{n}" for n in range(1, 9)],
    "router": [f"IC{ic}.{n}" for ic in range(10, 15) for n in (10, 11, 12, 13, 14)],
    # Display nach Lage auf der Platinen-RÜCKSEITE, je Reihe von links gezählt
    "dis1": [f"DIS1.{r}{n:02d}" for r in ("oben", "unten") for n in range(1, 15)],
    "dis2": [f"DIS2.{r}{n:02d}" for r in ("oben", "unten") for n in range(1, 15)],
}


class Tracer:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=3)
        deadline = time.time() + 6
        while time.time() < deadline:
            if b"TRACER READY" in self.ser.readline():
                return
        sys.exit("Kein READY vom Mega - ist der Sketch pin_tracer hochgeladen?")

    def cmd(self, text):
        self.ser.reset_input_buffer()
        self.ser.write((text + "\n").encode())
        return self.ser.readline().decode(errors="replace").strip()

    def trace(self):
        self.ser.reset_input_buffer()
        self.ser.write(b"T\n")
        parts = self.ser.readline().decode(errors="replace").split()
        if len(parts) < 3 or parts[0] != "T" or parts[-1] != "E":
            return None, []
        return parts[1], parts[2:-1]


def natural(name):
    """Sortierschlüssel: IC4.8 < IC4.11 < IC10.14 (Zahlen numerisch)."""
    return [int(t) if t.isdigit() else t.lower() for t in re.split(r"(\d+)", name)]


def describe(idle, hits):
    if idle is None:
        return "keine Antwort vom Mega"
    txt = f"Ruhepegel {'HIGH' if idle == 'H' else 'LOW'}; "
    if not hits:
        return txt + "keine Ader gefunden (nicht direkt am Kabel oder Spitze ohne Kontakt)"
    names = {"+": "direkt", "-": "invertiert", "!": "KONFLIKT"}
    return txt + ", ".join(f"{h[:-1]} ({names[h[-1]]})" for h in hits)


def main():
    if len(sys.argv) < 2:
        sys.exit("Aufruf: python tools\\pin_tracer.py COM8")
    base = [a[1:].upper() for a in sys.argv[2:] if a.startswith("+")]
    args = [a for a in sys.argv[2:] if not a.startswith("+")] or ["led"]
    targets = sorted(PRESETS.get(args[0].lower(), args), key=natural)
    tracer = Tracer(sys.argv[1])
    for b in base:
        if tracer.cmd(b) != "OK":
            tracer.cmd("R")
            sys.exit(f"Basis {b} fehlgeschlagen (Konflikt oder unbekannt).")

    logdir = Path(__file__).resolve().parent.parent / "logs"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"pin_tracer_{datetime.now():%Y-%m-%d_%H%M%S}.json"
    results = {"_basis": base}

    print("Verbunden. Messspitze = Kabel an D13. NIE an Punkte über 5 V halten!\n")
    i, last = 0, None
    while True:
        target = targets[i] if i < len(targets) else None
        prompt = f"[{target}] Spitze anhalten, Enter" if target else "Eigener Messpunkt (Name)"
        ans = input(f"{prompt} (Name = anderer Punkt, w = wiederholen, s/z/q): ").strip()
        low = ans.lower()
        if low == "q":
            break
        if low == "s":
            i += 1
            continue
        if low == "z":
            i = max(0, i - 1)
            continue
        if low == "w":
            if last is None:
                continue
            name = last
        elif ans:
            name = ans
        elif target:
            name = target
        else:
            continue

        idle, hits = tracer.trace()
        print(f"    {name}: {describe(idle, hits)}\n")
        results.setdefault(name, []).append({"ruhe": idle, "treffer": hits})
        logfile.write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding="utf-8")
        last = name
        if name == target:
            i += 1

    tracer.cmd("R")
    print(f"\nFertig. Ergebnis gespeichert in {logfile}")
    for name, runs in results.items():
        if name == "_basis":
            continue
        print(f"  {name:10} {describe(runs[-1]['ruhe'], runs[-1]['treffer'])}")
    input("\nEnter zum Schließen ...")


if __name__ == "__main__":
    main()
