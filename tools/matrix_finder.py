"""Interaktive Tasten-Zuordnung für das BSMS/2 Keyboard-Display-Board.

Voraussetzung: Sketch arduino/matrix_probe ist auf dem Mega, Board versorgt.
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\matrix_finder.py COM8

Ablauf:
  1. Referenz-Scans ohne Tastendruck (Störungen herausrechnen).
  2. Das Programm nennt nacheinander jede Taste. Taste drücken und HALTEN, bis
     "erkannt" erscheint, dann loslassen. s = Taste überspringen, q = beenden.
Ergebnis: logs/matrix_finder_<zeit>.json (wird nach jeder Taste gespeichert).
"""
import json
import msvcrt
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

KEYS = [
    ("SW1", "LIFT ON-OFF"), ("SW2", "SPIN RATE"), ("SW3", "SPIN ON-OFF"),
    ("SW4", "CHAN. SELECT"), ("SW5", "TRANS. P-DOWN"),
    ("SW6", "FIELD"), ("SW7", "LOCK PHASE"), ("SW8", "LOCK POWER"),
    ("SW9", "LOCK GAIN"), ("SW10", "SWEEP"), ("SW11", "SWEEP AMPL."),
    ("SW12", "HE LEVEL"), ("SW13", "(unbeschriftet)"), ("SW14", "LOCK ON-OFF"),
    ("SW15", "AUTO LOCK"), ("SW16", "LOCK DC"), ("SW17", "LOCK SHIFT"),
    ("SW18", "Z0"), ("SW19", "Z1"), ("SW20", "Z2"), ("SW21", "Z3"), ("SW22", "Z4"), ("SW23", "Z5"),
    ("SW24", "X"), ("SW25", "Y"), ("SW26", "X2-Y2"), ("SW27", "XY"), ("SW28", "X3"), ("SW29", "Y3"),
    ("SW30", "ONAXIS"), ("SW31", "AUTO SHIM"), ("SW32", "DIFF. MODE"),
    ("SW33", "STD BY"), ("SW34", "FINE"), ("SW35", "2nd"),
]

REF_SCANS = 8      # Referenz-Scans ohne Taste
STABLE = 3         # so viele gleiche Scans hintereinander = erkannt


class Probe:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=2)
        self.wires = {}  # idx -> P-Nummer
        deadline = time.time() + 6
        n = None
        while time.time() < deadline:
            line = self.ser.readline().decode(errors="replace").strip()
            if line.startswith("READY"):
                n = int(line.split()[1])
            elif line.startswith("W ") and n is not None:
                _, idx, p, _flags = line.split()
                self.wires[int(idx)] = int(p)
                if len(self.wires) == n:
                    return
        sys.exit("Kein READY vom Mega - ist der Sketch matrix_probe hochgeladen?")

    def scan(self):
        """Liefert (basis, {(idx, pegel): bitmap})."""
        self.ser.reset_input_buffer()
        self.ser.write(b"S")
        base, drives = None, {}
        while True:
            line = self.ser.readline().decode(errors="replace").strip()
            if not line:
                raise RuntimeError("Timeout beim Scan")
            if line.startswith("S "):
                base = int(line[2:], 16)
            elif line.startswith("D "):
                _, idx, lvl, bm = line.split()
                drives[(int(idx), lvl)] = int(bm, 16)
            elif line == "E":
                return base, drives

    def name(self, idx):
        return f"P{self.wires[idx]:02d}"


def bits(v):
    i = 0
    while v:
        if v & 1:
            yield i
        v >>= 1
        i += 1


def effects(probe, ref_base, ref_drives, noise_base, noise_drive, scan):
    """Abweichungen gegenüber der Referenz als sortiertes Tupel von Texten."""
    base, drives = scan
    out = []
    for i in bits((base ^ ref_base) & ~noise_base):
        out.append(f"direkt: {probe.name(i)} -> {'HIGH' if base >> i & 1 else 'LOW'}")
    for key, bm in drives.items():
        idx, lvl = key
        diff = (bm ^ ref_drives.get(key, bm)) & ~noise_drive.get(key, 0) & ~(1 << idx)
        # nur Änderungen, die nicht schon in der Basis stecken
        diff &= ~((base ^ ref_base))
        for i in bits(diff):
            out.append(f"{probe.name(idx)} {lvl} -> {probe.name(i)} {'HIGH' if bm >> i & 1 else 'LOW'}")
    return tuple(sorted(out))


def key_pressed():
    if msvcrt.kbhit():
        return msvcrt.getwch().lower()
    return None


def main():
    if len(sys.argv) < 2:
        sys.exit("Aufruf: python tools\\matrix_finder.py COM8")
    probe = Probe(sys.argv[1])
    print(f"Verbunden, {len(probe.wires)} Adern.\n")

    input(">>> KEINE Taste drücken. Enter = Referenzmessung starten ...")
    scans = [probe.scan() for _ in range(REF_SCANS)]
    ref_base, ref_drives = scans[0]
    noise_base, noise_drive = 0, {}
    for base, drives in scans[1:]:
        noise_base |= base ^ ref_base
        for k, bm in drives.items():
            noise_drive[k] = noise_drive.get(k, 0) | (bm ^ ref_drives[k])
    noisy = sum(bin(v).count("1") for v in noise_drive.values()) + bin(noise_base).count("1")
    print(f"Referenz fertig ({noisy} flackernde Messpunkte werden ignoriert).\n")

    logdir = Path(__file__).resolve().parent.parent / "logs"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"matrix_finder_{datetime.now():%Y-%m-%d_%H%M%S}.json"
    results = {"wires": {str(k): v for k, v in probe.wires.items()}, "keys": {}}

    for sw, label in KEYS:
        print(f">>> Taste {label} ({sw}) DRÜCKEN und HALTEN   [s = überspringen, q = beenden]")
        last, count, found = None, 0, None
        while found is None:
            c = key_pressed()
            if c == "q":
                break
            if c == "s":
                found = ()
                print("    übersprungen")
                break
            eff = effects(probe, ref_base, ref_drives, noise_base, noise_drive, probe.scan())
            if eff and eff == last:
                count += 1
            else:
                count = 1 if eff else 0
            last = eff
            if count >= STABLE:
                found = eff
        if c == "q":
            print("Abbruch.")
            break
        if found:
            print("    ERKANNT:")
            for e in found:
                print("      " + e)
            print("    -> loslassen")
            quiet = 0
            while quiet < STABLE:
                eff = effects(probe, ref_base, ref_drives, noise_base, noise_drive, probe.scan())
                quiet = quiet + 1 if not eff else 0
        results["keys"][sw] = {"label": label, "effects": list(found or [])}
        logfile.write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding="utf-8")
        print()

    print(f"\nFertig. Ergebnis gespeichert in {logfile}")
    print("\nZusammenfassung:")
    for sw, r in results["keys"].items():
        print(f"  {sw:5} {r['label']:16} {'; '.join(r['effects']) or '-'}")
    input("\nEnter zum Schließen ...")


if __name__ == "__main__":
    main()
