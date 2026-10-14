"""Interaktive Suche nach der LED-Ansteuerung des BSMS/2 Keyboard-Display-Boards.

Voraussetzung: Sketch arduino/led_probe ist auf dem Mega, Board versorgt, und
TP2 (P17, LED-Versorgung VS) hängt an VIN (9-V-Netzteil, ca. 8,6 V).
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\led_finder.py COM8 [L26 ...] [C46 C47 ...]

Optionale Basis-Befehle (z. B. L26 = P26 dauerhaft LOW) werden während des ganzen
Laufs gehalten, die Kandidaten blinken zusätzlich. Mit Basis werden zuerst die
Tasten-Zeilen P04-P14 auf HIGH geblinkt (sie sind auch die LED-Zeilen).
C<nr> beschränkt die Kandidaten auf die angegebenen Adern (Pegel automatisch).

Das Programm lässt nacheinander jede Kandidaten-Ader blinken (LOW <-> frei bzw.
HIGH <-> frei) und fragt, was auf dem Board blinkt. Blinkende LEDs fallen besser
auf als dauerhaft leuchtende.
Eingabe: Beschreibung (z. B. "LED in FIELD", "Bargraph links") oder Enter = nichts.
         z = zurück zur vorigen Ader, q = beenden.
Ergebnis: logs/led_finder_<zeit>.json (wird nach jeder Antwort gespeichert).
"""
import json
import sys
import threading
import time
from datetime import datetime
from pathlib import Path

import serial

# Im Ruhezustand HIGH liegende Adern (Board-Pull-ups) -> LOW blinken lassen,
# im Ruhezustand LOW liegende (keine Matrix-Zeile) -> HIGH blinken lassen.
CANDIDATES = [(p, "L") for p in (16, 18, 20, 22, 24, 25, 26, 27, 28, 29, 30, 31, 32,
                                 33, 34, 35, 36, 37, 38, 39, 46, 47, 48, 49, 50, 51,
                                 53, 54, 55, 56, 57)] + [(2, "H"), (52, "H")]
ROWS = [(p, "H") for p in (4, 6, 8, 10, 12, 14)]


class Probe:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=2)
        self.lock = threading.Lock()
        deadline = time.time() + 6
        while time.time() < deadline:
            if b"LED PROBE READY" in self.ser.readline():
                return
        sys.exit("Kein READY vom Mega - ist der Sketch led_probe hochgeladen?")

    def cmd(self, text):
        with self.lock:
            self.ser.write((text + "\n").encode())
            return self.ser.readline().decode(errors="replace").strip()


def blink(probe, p, lvl, stop):
    on = False
    while not stop.is_set():
        on = not on
        answer = probe.cmd(f"{lvl}{p}" if on else f"F{p}")
        if answer == "KONFLIKT":
            print(f"\n    !!! P{p}: Konflikt mit einem Board-Ausgang - übersprungen")
            break
        stop.wait(0.4)
    probe.cmd(f"F{p}")


def main():
    if len(sys.argv) < 2:
        sys.exit("Aufruf: python tools\\led_finder.py COM8")
    probe = Probe(sys.argv[1])
    probe.cmd("R")
    args = [a.upper() for a in sys.argv[2:]]
    base = [a for a in args if a[0] in "LH"]
    only = {int(a[1:]) for a in args if a[0] == "C"}
    for b in base:
        if probe.cmd(b) != "OK":
            probe.cmd("R")
            sys.exit(f"Basis-Befehl {b} fehlgeschlagen (Konflikt oder unbekannt).")
    used = {int(b[1:]) for b in base}
    cands = [c for c in (ROWS + CANDIDATES if base else CANDIDATES) if c[0] not in used]
    if only:
        cands = [c for c in cands if c[0] in only]

    logdir = Path(__file__).resolve().parent.parent / "logs"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"led_finder_{datetime.now():%Y-%m-%d_%H%M%S}.json"
    results = {"basis": base, "ruhe": "", "adern": {}}

    print(f"Verbunden. Basis: {' '.join(base) or '-'}\n")
    results["ruhe"] = input(">>> Was leuchtet JETZT schon (Ruhezustand)? Beschreibung oder Enter = nichts: ").strip()
    logfile.write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding="utf-8")

    i = 0
    while i < len(cands):
        p, lvl = cands[i]
        stop = threading.Event()
        t = threading.Thread(target=blink, args=(probe, p, lvl, stop), daemon=True)
        t.start()
        ans = input(f"[{i + 1}/{len(cands)}] P{p:02d} blinkt ({'LOW' if lvl == 'L' else 'HIGH'}). "
                    f"Was blinkt? (Enter = nichts, z = zurück, q = Ende): ").strip()
        stop.set()
        t.join()
        if ans.lower() == "q":
            break
        if ans.lower() == "z":
            i = max(0, i - 1)
            continue
        results["adern"][f"P{p:02d}"] = {"pegel": lvl, "beobachtung": ans}
        logfile.write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding="utf-8")
        i += 1

    probe.cmd("R")
    print(f"\nFertig. Ergebnis gespeichert in {logfile}")
    for k, v in results["adern"].items():
        if v["beobachtung"]:
            print(f"  {k} {v['pegel']}: {v['beobachtung']}")
    input("\nEnter zum Schließen ...")


if __name__ == "__main__":
    main()
