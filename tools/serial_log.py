"""Liest die serielle Ausgabe des Mega mit und schreibt sie mit Zeitstempel in eine Logdatei.

Aufruf (Windows-Python): python serial_log.py COM8 <logdatei> [sekunden] [befehl]
  sekunden: Laufzeit (Standard 60); befehl: optionaler String, der nach dem Start gesendet wird.
Oeffnen des Ports setzt den Mega zurueck -> Sketch startet neu.
"""
import sys, time, serial

port, logfile = sys.argv[1], sys.argv[2]
duration = float(sys.argv[3]) if len(sys.argv) > 3 else 60
command = sys.argv[4] if len(sys.argv) > 4 else None

with serial.Serial(port, 115200, timeout=0.2) as ser, open(logfile, "w", encoding="utf-8", buffering=1) as log:
    start = time.time()
    if command:
        time.sleep(2.5)
        ser.write(command.encode())
    while time.time() - start < duration:
        line = ser.readline().decode("utf-8", "replace").rstrip()
        if line:
            log.write(f"{time.time() - start:7.2f}  {line}\n")
