"""Kompletter LED-Test: geht Schritt für Schritt durch alle LEDs und sagt, was neu leuchten sollte.

Voraussetzung: Sketch arduino/led_test ist auf dem Mega, Board versorgt, TP2 (P17) an VIN.
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\led_test.py COM8

Tasten: Leertaste/n = nächster Schritt, b = zurück, a = Automatik an/aus,
        0 = Anfang, m = zu den Tasten-LEDs springen,
        f = beim aktuellen Schritt "fehlt / leuchtet nicht" vermerken, q = beenden.
Ergebnis: logs/led_test_<zeit>.json (vermerkte Schritte).
"""
import json
import msvcrt
import sys
import threading
import time
from datetime import datetime
from pathlib import Path

import serial

# 595-Kette: Position k = die k-te LED, die beim Füllen dazukommt (bestätigt mit led_test 2026-10-12).
CHAIN = {k: f"Bargraph LED25 (grün) Segment {k}" for k in range(1, 11)}
CHAIN.update({k: f"Bargraph LED26 (rot) Segment {k - 10}" for k in range(11, 21)})
CHAIN.update(dict(zip(range(21, 41), ['LED5', 'LED6', 'LED14', 'LED15', 'LED1', 'LED7', 'LED4', 'LED10', 'LED13', 'LED18', 'LED in Taste TRANS. P-DOWN', 'LED19', 'LED2', 'LED8', 'LED11', 'LED16', 'LED3', 'LED9', 'LED12', 'LED17'])))


MATRIX = [
    ["SW1 LIFT ON-OFF", "SW2 SPIN RATE", "SW3 SPIN ON-OFF", "(keine Taste)", "(keine Taste)", "(keine Taste)"],
    ["SW6 FIELD", "SW7 LOCK PHASE", "SW8 LOCK POWER", "SW9 LOCK GAIN", "SW10 SWEEP", "SW11 SWEEP AMPL."],
    ["SW12 HE LEVEL", "SW13 (leer)", "SW14 LOCK ON-OFF", "SW15 AUTO LOCK", "SW16 LOCK DC", "SW17 LOCK SHIFT"],
    ["SW18 Z0", "SW19 Z1", "SW20 Z2", "SW21 Z3", "SW22 Z4", "SW23 Z5"],
    ["SW24 X", "SW25 Y", "SW26 X2-Y2", "SW27 XY", "SW28 X3", "SW29 Y3"],
    ["SW30 ONAXIS", "SW31 AUTO SHIM", "SW32 DIFF. MODE", "SW33 STD BY", "SW34 FINE", "SW35 2nd"],
    ["LED20 (rot)", "LED21 (grün)", "LED22 (rot)", "LED27 (nicht bestückt)", "LED23 (rot)", "LED24 (rot)"],
]
ROW_NAMES = ["P16", "P14", "P12", "P10", "P08", "P06", "P04"]


def describe(step):
    if step == 0:
        return "Alles aus (Kette gelöscht)"
    if step <= 40:
        return f"Kette Position {step:2d}: NEU dazu -> {CHAIN[step]}   (insgesamt {step} an)"
    i = step - 41
    r, c = divmod(i, 6)
    return f"Matrix Zeile {ROW_NAMES[r]} Spalte {c + 1}: NUR -> {MATRIX[r][c]}"


def main():
    if len(sys.argv) < 2:
        sys.exit("Aufruf: python tools\\led_test.py COM8")
    ser = serial.Serial(sys.argv[1], 115200, timeout=0.2)
    deadline = time.time() + 6
    while time.time() < deadline:
        if b"LED TEST READY" in ser.readline():
            break
    else:
        sys.exit("Kein READY vom Mega - ist der Sketch led_test hochgeladen?")

    logdir = Path(__file__).resolve().parent.parent / "logs"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"led_test_{datetime.now():%Y-%m-%d_%H%M%S}.json"
    missing = {}
    current = [0]
    stop = threading.Event()

    def reader():
        while not stop.is_set():
            line = ser.readline().decode(errors="replace").strip()
            if line.startswith("STEP "):
                current[0] = int(line[5:])
                print(f"[{current[0]:2d}] {describe(current[0])}")

    threading.Thread(target=reader, daemon=True).start()
    print("Leertaste/n = weiter, b = zurück, a = Automatik, 0 = Anfang, m = Tasten-LEDs, "
          "f = fehlt vermerken, q = Ende\n")
    print(f"[ 0] {describe(0)}")
    while True:
        ch = msvcrt.getwch().lower()
        if ch == "q":
            break
        if ch == " ":
            ch = "n"
        if ch in "nba0m":
            ser.write(ch.encode())
        elif ch == "f":
            missing[current[0]] = describe(current[0])
            logfile.write_text(json.dumps(missing, indent=2, ensure_ascii=False), encoding="utf-8")
            print(f"     -> vermerkt: Schritt {current[0]} fehlt")
    stop.set()
    print(f"\nVermerkt ({len(missing)}):")
    for k, v in sorted(missing.items()):
        print(f"  [{k:2d}] {v}")
    if missing:
        print(f"Gespeichert in {logfile}")
    input("Enter zum Schließen ...")


if __name__ == "__main__":
    main()
