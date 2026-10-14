# BSMS Keyboard Boss

Eine ausgemusterte **Bruker-BSMS/2-Tastatur** (Bedieneinheit eines Bruker-NMR) wird per Reverse-Engineering
zum USB-HID-/MIDI-Controller umgebaut. Zum Testen hängt ein Arduino Mega 2560 am 64-poligen Flachbandkabel des Keyboard-Display-Boards.

**Stand:** Die gesamte Bedienhardware läuft am Arduino Mega:
- alle 35 Tasten
- alle Tasten- und Status-LEDs, das Router-Display und die Bargraphs (595-Kette)
- beide HDSP-2112-Displays (Text, Umlaute, Pixel-Emojis, Animation)
- der optische Drehregler

Als Nächstes kommen USB-HID bzw. MIDI und eine eigene Platine.

- `PINOUT.md`: komplette Belegung der 64 Kabeladern, LED-Matrix, 595-Kette, Displays, Encoder
- `CLAUDE.md`: Projekt-Logbuch mit allen Messungen, Erkenntnissen und Planungen
- `arduino/`: Sketches (`keys` = Demo-Firmware für das ganze Board, dazu Mess- und Test-Sketches)
- `tools/`: Python-Werkzeuge (u. a. `keyboard_gui.py` = grafische Oberfläche, interaktive Messprogramme)
- `logs/`: Mitschnitte der Messläufe
- `Fotos_web/`: Fotos der geöffneten Tastatur (verkleinert)
- `Manuals/`: Original-Handbücher von Bruker
