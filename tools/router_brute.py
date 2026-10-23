"""Brute-Force-Suche: Lassen sich Router-LEDs irgendwie einzeln schalten?

Voraussetzung: Sketch arduino/router_brute ist auf dem Mega, Board versorgt, TP2 (P17) an VIN.
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\router_brute.py COM8 wackel [k ...]  (Einsen in der Kette je Durchgang, Vorgabe 40 0)
    python tools\\router_brute.py COM8 zufall [ms]     (Haltezeit je Schritt, Vorgabe 400)
    python tools\\router_brute.py COM8 einzeln [ms]    (Anzeigedauer je LED, Vorgabe 1500)

wackel: läuft automatisch. Die Kette wird mit k Einsen gefüllt (40 = alle Router-LEDs an, 0 = alle aus),
        dann flackert jede Ader am Mega 2 s lang schnell (4 Hz), danach kommt die nächste.
        Zu Beginn flackert zur Probe P28 (/OE): So sieht ein Treffer aus.
        Nur auf die OBEREN LEDs achten (Router-LEDs, Bargraphs, LED in TRANS. P-DOWN). Unten flackern
        bei den Zeilen-Adern die Tasten-LEDs, das ist normal.
        Flackert oben etwas: beliebige Taste drücken. Wegen der Reaktionszeit werden die aktuelle und
        die vorige Ader vermerkt. Am Ende werden alle Verdächtigen einzeln wiederholt. q = Abbruch.
zufall: Die vier Kettensignale (P28, P55, P56, P57) bekommen endlos Zufallsfolgen. Normal sind nur
        Muster "die ersten k an" (Bargraph grün füllt sich zuerst). Leuchtet ein Muster mit Lücke,
        z. B. eine einzelne LED weiter hinten: sofort eine Taste drücken. Die letzten Schritte werden
        gespeichert. q = Ende.
einzeln: Test der neuen Datenleitung DLED = P60 (an D14). Jede der 40 Kettenpositionen leuchtet der
        Reihe nach ALLEIN, das Programm zeigt an, welche LED erwartet wird. Leuchtet etwas anderes
        oder mehr als eine LED: beliebige Taste drücken. q = Ende.
Ergebnis: logs/router_brute_<modus>_<zeit>.json (wird nach jeder Antwort gespeichert).
"""
import json
import msvcrt
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

# Ader -> (Funktion, erwarteter Effekt beim Wackeln)
WIRES = {
    4: ("LED-Zeile Status (LED20-24)", "Status-LEDs LED20-24 blinken mit (nur mit Netzteil)"),
    6: ("Zeile 6 (SW30-35)", "Tasten-LEDs dieser Zeile blinken mit (nur mit Netzteil)"),
    8: ("Zeile 5 (SW24-29)", "Tasten-LEDs dieser Zeile blinken mit (nur mit Netzteil)"),
    10: ("Zeile 4 (SW18-23)", "Tasten-LEDs dieser Zeile blinken mit (nur mit Netzteil)"),
    12: ("Zeile 3 (SW12-17)", "Tasten-LEDs dieser Zeile blinken mit (nur mit Netzteil)"),
    14: ("Zeile 2 (SW6-11)", "Tasten-LEDs dieser Zeile blinken mit (nur mit Netzteil)"),
    16: ("Zeile 1 (SW1-3)", "Tasten-LEDs dieser Zeile blinken mit (nur mit Netzteil)"),
    18: ("Display DIS1 /CE", "nichts"),
    20: ("Display DIS2 /CE", "nichts"),
    22: ("Display /FL", "nichts"),
    24: ("Display A0", "nichts"),
    26: ("FREI, unbekannt", "??? (Kandidat!)"),
    25: ("Display A1", "nichts"),
    28: ("/OE Kette + 245", "alle Router-LEDs blinken gemeinsam"),
    27: ("Display A2", "nichts"),
    30: ("Display D7", "nichts"),
    29: ("Display A3", "nichts"),
    32: ("Display D6", "nichts"),
    31: ("Display A4", "nichts"),
    33: ("Display D5", "nichts"),
    34: ("Display D4", "nichts"),
    35: ("Display D3", "nichts"),
    36: ("Display D2", "nichts"),
    37: ("Display D1", "nichts"),
    38: ("Display D0", "nichts"),
    39: ("Display /RST", "Displays werden gelöscht, sonst nichts"),
    40: ("Tasten-Spalte 6", "nichts"),
    41: ("Tasten-Spalte 5", "nichts"),
    42: ("Tasten-Spalte 4", "nichts"),
    43: ("Tasten-Spalte 3", "nichts"),
    44: ("Tasten-Spalte 2", "nichts"),
    45: ("Tasten-Spalte 1", "nichts"),
    46: ("LED-Bus Spalte 1", "nichts (Zeilen sind aus)"),
    47: ("LED-Bus Spalte 2", "nichts (Zeilen sind aus)"),
    48: ("LED-Bus Spalte 3", "nichts (Zeilen sind aus)"),
    49: ("LED-Bus Spalte 4", "nichts (Zeilen sind aus)"),
    50: ("LED-Bus Spalte 5", "nichts (Zeilen sind aus)"),
    51: ("LED-Bus Spalte 6", "nichts (Zeilen sind aus)"),
    52: ("TP1, 0,7 kOhm nach GND", "??? (Kandidat)"),
    53: ("Display /RD", "nichts"),
    54: ("Display /WR", "nichts"),
    58: ("Direkttaste TRANS. P-DOWN", "nichts"),
    59: ("Direkttaste CHAN. SELECT", "nichts"),
    55: ("/SRCLR Kette", "nichts (löscht nur das Schieberegister, ohne Übernahme unsichtbar)"),
    57: ("RCLK Kette", "nichts bzw. alles aus, falls P55 vorher gelöscht hat"),
    56: ("SRCLK Kette", "bei k < 40 kommen LEDs am Ende der Reihe dazu (erst nach Übernahme sichtbar)"),
}


class Brute:
    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=0.2)
        deadline = time.time() + 6
        while time.time() < deadline:
            if b"ROUTER BRUTE READY" in self.ser.readline():
                return
        sys.exit("Kein READY vom Mega - ist der Sketch router_brute hochgeladen?")

    def cmd(self, text, timeout=8):
        self.ser.write((text + "\n").encode())
        deadline = time.time() + timeout
        while time.time() < deadline:
            answer = self.ser.readline().decode(errors="replace").strip()
            if answer:
                return answer
        return "keine Antwort"

    def cmd_watch(self, text, timeout=8):
        """Wie cmd, merkt sich aber eine Taste, die währenddessen gedrückt wird."""
        self.ser.write((text + "\n").encode())
        deadline = time.time() + timeout
        key = None
        while time.time() < deadline:
            if msvcrt.kbhit():
                key = msvcrt.getwch()
            answer = self.ser.readline().decode(errors="replace").strip()
            if answer:
                return answer, key
        return "keine Antwort", key


def save(logfile, results):
    logfile.write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding="utf-8")


HALF_MS, COUNT = 125, 8   # 4 Hz, 2 s je Ader


def wackel(brute, ks, logfile):
    results = {"modus": "wackel", "durchgaenge": ks, "treffer": [], "nachtest": []}
    brute.cmd(f"K{ks[0]}")
    print("Probe: P28 (/OE) flackert jetzt. So sieht ein Treffer aus (hier flackert alles oben).")
    brute.cmd(f"B28,{HALF_MS},{COUNT}")
    input("Enter = Start. Ab jetzt nur noch eine Taste drücken, wenn OBEN etwas flackert. ")

    candidates = []
    for k in ks:
        print(f"\n--- Durchgang mit {k} Einsen in der Kette ({'alle oben an' if k == 40 else 'oben ' + str(k) + ' an'}) ---")
        prev = None
        for p, (name, _) in WIRES.items():
            if p == 28:
                continue
            brute.cmd(f"K{k}")
            print(f"P{p:02d} {name}")
            answer, key = brute.cmd_watch(f"B{p},{HALF_MS},{COUNT}")
            if answer != "OK":
                print(f"   -> {answer}")
            if key is not None:
                while msvcrt.kbhit():
                    msvcrt.getwch()
                if key.lower() == "q":
                    save(logfile, results)
                    return
                hit = {"k": k, "ader": f"P{p:02d}", "vorige": f"P{prev:02d}" if prev else None}
                print(f"   *** vermerkt: P{p:02d}" + (f" oder P{prev:02d}" if prev else ""))
                results["treffer"].append(hit)
                save(logfile, results)
                for c in (prev, p):
                    if c and (k, c) not in candidates:
                        candidates.append((k, c))
            prev = p
    save(logfile, results)

    if candidates:
        print(f"\n=== Nachtest: {len(candidates)} verdächtige Adern einzeln ===")
    for k, p in candidates:
        while True:
            brute.cmd(f"K{k}")
            input(f"\nP{p:02d} {WIRES[p][0]} (Kette {k}): Enter = flackern lassen ")
            answer = brute.cmd(f"B{p},{HALF_MS},{COUNT}")
            ans = input("   Hat oben etwas geflackert? (n = nein, w = nochmal, sonst Beschreibung) ").strip()
            if ans.lower() != "w":
                break
        results["nachtest"].append({"k": k, "ader": f"P{p:02d}", "sketch": answer, "beobachtung": ans})
        save(logfile, results)
    brute.cmd(f"K{ks[-1]}")


def zufall(brute, hold_ms, logfile):
    results = {"modus": "zufall", "haltezeit_ms": hold_ms, "treffer": []}
    print("Zufallsfolgen laufen. Taste drücken, sobald ein Muster mit Lücke leuchtet (q = Ende).\n")
    while True:
        brute.ser.write(f"Z{hold_ms}\n".encode())
        last = []
        key = None
        while key is None:
            line = brute.ser.readline().decode(errors="replace").strip()
            if line.startswith("Z "):
                last = (last + [line[2:]])[-10:]
                print(f"\r   Schritt {line[2:]:20s}", end="", flush=True)
            if msvcrt.kbhit():
                key = msvcrt.getwch()
        brute.ser.write(b"x")
        while "STOP" not in brute.ser.readline().decode(errors="replace"):
            pass
        print()
        if key.lower() == "q":
            break
        ans = input("   Was leuchtet? (Beschreibung, Enter = weiter ohne Eintrag) ").strip()
        if ans:
            results["treffer"].append({"letzte_schritte": last, "beobachtung": ans})
            save(logfile, results)
    save(logfile, results)


# Kettenposition -> LED (bestätigt mit led_test 2026-10-12)
CHAIN = {k: f"Bargraph LED25 (grün) Segment {k}" for k in range(1, 11)}
CHAIN.update({k: f"Bargraph LED26 (rot) Segment {k - 10}" for k in range(11, 21)})
CHAIN.update(dict(zip(range(21, 41), ["LED5", "LED6", "LED14", "LED15", "LED1", "LED7", "LED4", "LED10",
                                      "LED13", "LED18", "LED in Taste TRANS. P-DOWN", "LED19", "LED2", "LED8",
                                      "LED11", "LED16", "LED3", "LED9", "LED12", "LED17"])))


def einzeln(brute, show_ms, logfile):
    results = {"modus": "einzeln", "anzeige_ms": show_ms, "auffaellig": []}
    print("Jede Position leuchtet allein. Taste drücken, wenn etwas anderes oder mehr leuchtet (q = Ende).\n")
    for pos in range(1, 41):
        answer = brute.cmd(f"P{pos}")
        print(f"Position {pos:2d}: erwartet {CHAIN[pos]}" + ("" if answer == "OK" else f"  -> {answer}"))
        key = None
        deadline = time.time() + show_ms / 1000
        while time.time() < deadline and key is None:
            if msvcrt.kbhit():
                key = msvcrt.getwch()
            time.sleep(0.02)
        if key is None:
            continue
        if key.lower() == "q":
            break
        ans = input("   Was leuchtet stattdessen? ").strip()
        results["auffaellig"].append({"position": pos, "erwartet": CHAIN[pos], "beobachtung": ans})
        save(logfile, results)
    brute.cmd("P0")
    save(logfile, results)


def main():
    if len(sys.argv) < 3 or sys.argv[2] not in ("wackel", "zufall", "einzeln"):
        sys.exit("Aufruf: python tools\\router_brute.py COM8 wackel [k ...]  |  zufall [ms]  |  einzeln [ms]")
    mode = sys.argv[2]
    if mode == "wackel":
        arg = [int(a) for a in sys.argv[3:]] or [40, 0]
    else:
        arg = int(sys.argv[3]) if len(sys.argv) > 3 else (400 if mode == "zufall" else 1500)
    if mode == "zufall" or mode == "wackel" and max(arg) > 10:
        print("ACHTUNG: Hier leuchten bis zu 40 LEDs gleichzeitig. Ohne Netzteil kommt der Strom")
        print("rückwärts über den Spannungsregler aus USB (max. ca. 500 mA). Netzteil am Mega, P17 an VIN?")
        if input("Netzteil dran? (j/n) ").strip().lower() != "j":
            sys.exit("Abgebrochen. Ohne Netzteil höchstens 'wackel 10' oder kleiner.")
    brute = Brute(sys.argv[1])

    logdir = Path(__file__).resolve().parent.parent / "logs"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"router_brute_{mode}_{datetime.now():%Y-%m-%d_%H%M%S}.json"
    try:
        if mode == "wackel":
            wackel(brute, arg, logfile)
        elif mode == "zufall":
            zufall(brute, arg, logfile)
        else:
            einzeln(brute, arg, logfile)
    finally:
        brute.ser.write(b"R\n")
        time.sleep(0.2)
        print(f"\nFertig, alles freigegeben. Ergebnis gespeichert in {logfile}")
        input("Enter zum Schließen ...")


if __name__ == "__main__":
    main()
