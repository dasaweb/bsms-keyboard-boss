"""Grafisches Tasten-Feedback für das BSMS/2 Keyboard-Display-Board.

Voraussetzung: Sketch arduino/keys ist auf dem Mega, Board versorgt.
Aufruf (Windows, PowerShell, im Projektordner):
    python tools\\keyboard_gui.py COM8

Gedrückte Tasten leuchten auf. Jede Taste, die schon einmal gedrückt wurde,
bekommt einen grünen Rand ("getestet"). Taste R im Fenster setzt das zurück.
Der grüne Punkt jeder Taste zeigt den echten Zustand ihrer LED auf dem Board, ebenso
das Router-Display oben (Bargraphs LED25/26, LED1-19) und die Status-LEDs LED20-24.
Die Tastenfunktionen stecken im Sketch (ONAXIS/2nd = Kette +1/-1, STD BY = löschen,
TRANS. P-DOWN / CHAN. SELECT = Animation, HE LEVEL ... LOCK SHIFT = LED20-24).
Ein Mausklick auf eine Taste wirkt wie ein Tastendruck.
Das Display zeigt den echten Text der beiden HDSP-2112 (links DIS2, rechts DIS1); über das
Eingabefeld rechts lässt sich eigener Text auf die Displays schicken. FINE = Helligkeit.
Layout nach dem Display-Board (Foto) im Stil von Figure 16 aus z31476.
"""
import calendar
import math
import queue
import sys
import threading
import time

import serial
import tkinter as tk

# (SW, Beschriftung, Zeile, Spalte, Farbe)  Farbe: g = grau, b = schwarz, o = orange
KEYS = [
    (1, "LIFT\nON-OFF", 0, 0, "g"), (2, "SPIN\nRATE", 0, 1, "g"), (3, "SPIN\nON-OFF", 0, 2, "g"),
    (4, "CHAN.\nSELECT", 0, 4, "g"), (5, "TRANS.\nP-DOWN", 0, 5, "g"),
    (6, "FIELD", 1, 0, "b"), (7, "LOCK\nPHASE", 1, 1, "b"), (8, "LOCK\nPOWER", 1, 2, "b"),
    (9, "LOCK\nGAIN", 1, 3, "b"), (10, "SWEEP", 1, 4, "b"), (11, "SWEEP\nAMPL.", 1, 5, "b"),
    (12, "HE\nLEVEL", 2, 0, "g"), (13, "", 2, 1, "b"), (14, "LOCK\nON-OFF", 2, 2, "b"),
    (15, "AUTO\nLOCK", 2, 3, "b"), (16, "LOCK\nDC", 2, 4, "b"), (17, "LOCK\nSHIFT", 2, 5, "b"),
    (18, "Z⁰", 3, 0, "b"), (19, "Z¹", 3, 1, "b"), (20, "Z²", 3, 2, "b"),
    (21, "Z³", 3, 3, "b"), (22, "Z⁴", 3, 4, "b"), (23, "Z⁵", 3, 5, "b"),
    (24, "X", 4, 0, "b"), (25, "Y", 4, 1, "b"), (26, "X²-Y²", 4, 2, "b"),
    (27, "XY", 4, 3, "b"), (28, "X³", 4, 4, "b"), (29, "Y³", 4, 5, "b"),
    (30, "ONAXIS", 5, 0, "b"), (31, "AUTO\nSHIM", 5, 1, "g"), (32, "DIFF.\nMODE", 5, 2, "g"),
    (33, "STD BY", 5, 3, "g"), (34, "FINE", 5, 4, "g"), (35, "2nd", 5, 5, "o"),
]

COLORS = {  # Tastenfarbe normal / gedrückt, Schriftfarbe
    "g": ("#a9adb0", "#e4e7ea", "#ffffff"),
    "b": ("#26282b", "#4a4f55", "#f2f2f2"),
    "o": ("#d2542b", "#ff8a5c", "#ffffff"),
}
HOUSING = "#d8d9d4"
PANEL = "#c4c6c1"
LED_OFF = "#2f4a2f"
LED_ON = "#5dff6a"
TESTED = "#2fbf4a"
RED_OFF, RED_ON = "#4a1e1e", "#ff3b3b"
GREEN_OFF, GREEN_ON = "#1f3a22", "#4dff6a"

# 595-Kette: Position 21-40 -> LED-Nummer (0 = LED in Taste TRANS. P-DOWN)
CHAIN_LEDS = [5, 6, 14, 15, 1, 7, 4, 10, 13, 18, 0, 19, 2, 8, 11, 16, 3, 9, 12, 17]
# Status-LEDs: Spalte -> Name, Farbe (Spalte 4 = LED27 unbestückt)
STATUS = {1: ("LED20", "r"), 2: ("LED21", "g"), 3: ("LED22", "r"), 5: ("LED23", "r"), 6: ("LED24", "r")}

# Router-LEDs nach Platine/Frontblende (Fotos 2026-10-12): Nummer -> (Beschriftung, Farbe, Form)
# Farbe g = grün, y = gelb, r = rot; Form "r" = Pfeil nach rechts, "l" = Pfeil nach links
PULSE = "⊓"
ROUTER = {
    1: ("OBS", "g", ""), 2: (PULSE, "y", ""), 3: (PULSE, "y", ""), 4: ("MIS", "r", ""), 5: ("X", "g", "r"),
    6: ("H", "g", "l"), 7: ("OBS", "g", ""), 8: (PULSE, "y", ""), 9: (PULSE, "y", ""), 10: ("MIS", "r", ""),
    11: (PULSE, "y", ""), 12: (PULSE, "y", ""), 13: ("MIS", "r", ""), 14: ("Y", "g", "r"),
    15: ("Z", "g", "l"), 16: (PULSE, "y", ""), 17: (PULSE, "y", ""), 18: ("MIS", "r", ""),
    19: ("ADC", "r", ""),
}
YELLOW_OFF, YELLOW_ON = "#3d3417", "#ffc93b"
LED_COLORS = {"g": (GREEN_OFF, GREEN_ON), "y": (YELLOW_OFF, YELLOW_ON), "r": (RED_OFF, RED_ON)}

# Tasten mit Sonderfunktion im Sketch keys: SW -> (Markierung, Erklärung)
SPECIAL = {
    30: ("+1", "ONAXIS: Kette +1 (halten)"), 35: ("−1", "2nd: Kette −1 (halten)"), 33: ("CLR", "STD BY: Kette löschen"),
    5: ("ANIM", "TRANS. P-DOWN: Animation ganze Kette"), 4: ("BARS", "CHAN. SELECT: Animation Bars"),
    12: ("L20", "HE LEVEL: LED20 blinkt"), 13: ("L21", "leere Taste: LED21 blinkt"),
    14: ("L22", "LOCK ON-OFF: LED22 blinkt"), 16: ("L23", "LOCK DC: LED23 blinkt"),
    17: ("L24", "LOCK SHIFT: LED24 blinkt"), 34: ("HELL", "FINE: Display heller (halten)"),
    10: ("LAUF", "SWEEP: Display-Animation an/aus"),
    11: ("GAP", "SWEEP AMPL.: Spalt 0-5 Pixel"),
    32: ("UHR", "DIFF. MODE: Datum/Uhrzeit"),
}
BRIGHT_PCT = ["100", "80", "53", "40", "27", "20", "13", "0"]
BADGE = "#ffd54a"
GLOW = "#9dffb0"

KW, KH, GAP = 92, 58, 14          # Tastenbreite, -höhe, Abstand
LEFT = 44
ROW_Y = [140, 214, 288, 462, 536, 610]   # Oberkante je Zeile (Display zwischen Zeile 3 und 4)
WIDTH = LEFT * 2 + 6 * KW + 5 * GAP
HEIGHT = 850


class KeyboardGUI:
    def __init__(self, root, port):
        self.root = root
        self.events = queue.Queue()
        self.pressed = set()
        self.tested = set()
        self.items = {}
        self.leds = set()
        self.chain_items = {}    # Position 1-40 -> (Item, Farbe aus, Farbe an)
        self.status_items = {}
        self.status_mode = {}
        self.blink_on = True
        self.ser = None

        root.title("BSMS/2 Keyboard – Tastentest")
        root.configure(bg=HOUSING)
        self.c = tk.Canvas(root, width=WIDTH, height=HEIGHT, bg=HOUSING, highlightthickness=0)
        self.c.pack(side="left")
        side = tk.Frame(root, bg=HOUSING)
        side.pack(side="left", fill="y", padx=(0, 16), pady=16)
        self.status = tk.Label(side, text="Verbinde ...", bg=HOUSING, font=("Segoe UI", 11, "bold"), anchor="w",
                               wraplength=260, justify="left")
        self.status.pack(fill="x")
        self.counter = tk.Label(side, text="", bg=HOUSING, font=("Segoe UI", 11), anchor="w")
        self.counter.pack(fill="x", pady=(4, 10))
        tk.Label(side, text="Ereignisse", bg=HOUSING, font=("Segoe UI", 10), anchor="w").pack(fill="x")
        self.log = tk.Listbox(side, width=30, height=30, font=("Consolas", 10), bg="#f4f4f1", relief="flat")
        self.log.pack(fill="y", expand=True)
        tk.Label(side, text="Text auf die Displays (Enter):", bg=HOUSING, font=("Segoe UI", 10),
                 anchor="w").pack(fill="x", pady=(8, 0))
        self.entry = tk.Entry(side, font=("Consolas", 12), width=18)
        self.entry.pack(fill="x")
        self.entry.bind("<Return>", lambda e: self.send_text())
        legend = "Sonderfunktionen (gelb markiert):\n" + "\n".join(
            f"  {t:5} {d}" for t, d in SPECIAL.values())
        tk.Label(side, text=legend, bg=HOUSING, font=("Consolas", 8), justify="left",
                 anchor="w").pack(fill="x", pady=(8, 0))
        tk.Label(side, text="R = Testmarkierungen zurücksetzen", bg=HOUSING,
                 font=("Segoe UI", 9), fg="#555").pack(fill="x", pady=(8, 0))
        root.bind("<Key-r>", lambda e: self.reset_tested() if e.widget is not self.entry else None)
        root.bind("<Key-R>", lambda e: self.reset_tested() if e.widget is not self.entry else None)

        self.draw_panel()
        self.update_counter()

        threading.Thread(target=self.reader, args=(port,), daemon=True).start()
        root.after(10, self.poll)
        root.after(400, self.blink)
        root.after(60000, self.time_tick)

    # ---------- Zeichnen ----------
    def rrect(self, x1, y1, x2, y2, r, **kw):
        pts = [x1 + r, y1, x2 - r, y1, x2, y1, x2, y1 + r, x2, y2 - r, x2, y2,
               x2 - r, y2, x1 + r, y2, x1, y2, x1, y2 - r, x1, y1 + r, x1, y1]
        return self.c.create_polygon(pts, smooth=True, **kw)

    def key_xy(self, row, col):
        x = LEFT + col * (KW + GAP)
        return x, ROW_Y[row]

    def draw_panel(self):
        c = self.c
        self.rrect(18, 18, WIDTH - 18, HEIGHT - 18, 26, fill=PANEL, outline="#a7a9a4", width=2)

        # Router-Display nach Frontblende: oben LED1-5 | FWD (LED25) | LED6-10,
        # unten LED11-14 | REFL (LED26) | LED19 (ADC) | LED15-18
        self.rrect(LEFT, 36, WIDTH - LEFT, 128, 10, fill="#d9dbd6", outline="#9a9c97")
        lw, lh, pitch = 22, 9, 27
        x_left, x_right = LEFT + 14, WIDTH - LEFT - 14 - 5 * pitch + (pitch - lw)
        led_xy = {}
        for i in range(5):
            led_xy[1 + i] = (x_left + i * pitch, 48)
            led_xy[6 + i] = (x_right + i * pitch, 48)
        for i in range(4):
            led_xy[11 + i] = (x_left + i * pitch, 90)
            led_xy[15 + i] = (x_right + (i + 1) * pitch, 90)
        led_xy[19] = (x_right - 52, 68)
        item_of = {}
        for n, (x, y) in led_xy.items():
            label, color, shape = ROUTER[n]
            off, on = LED_COLORS[color]
            if shape == "r":
                pts = [x, y, x + lw - 5, y, x + lw, y + lh / 2, x + lw - 5, y + lh, x, y + lh]
            elif shape == "l":
                pts = [x + 5, y, x + lw, y, x + lw, y + lh, x + 5, y + lh, x, y + lh / 2]
            else:
                pts = [x, y, x + lw, y, x + lw, y + lh, x, y + lh]
            item_of[n] = (c.create_polygon(pts, fill=off, outline="#555"), off, on)
            c.create_text(x + lw / 2, y + lh + 9, text=label, font=("Segoe UI", 8, "bold"), fill="#55575a")
            c.create_text(x + lw / 2, y - 6, text=str(n), font=("Segoe UI", 6), fill="#9a9c97")
        bx = WIDTH / 2 - 125
        for k in range(1, 21):
            i = (k - 1) % 10
            base = 64 if k <= 10 else 106
            h = 6 + i * 1.2
            x = bx + i * 19
            off, on = (GREEN_OFF, GREEN_ON) if k <= 10 else (RED_OFF, RED_ON)
            self.chain_items[k] = (c.create_rectangle(x, base - h, x + 15, base, fill=off, outline=""), off, on)
        c.create_text(bx + 92, 73, text="FWD  (LED25)", font=("Segoe UI", 8, "bold"), fill="#55575a")
        c.create_text(bx + 92, 115, text="REFL  (LED26)", font=("Segoe UI", 8, "bold"), fill="#55575a")
        for k in range(21, 41):
            n = CHAIN_LEDS[k - 21]
            if n:                     # 0 = TRANS. P-DOWN: Punkt auf der Taste, siehe unten
                self.chain_items[k] = item_of[n]

        # Tasten
        for sw, label, row, col, color in KEYS:
            x, y = self.key_xy(row, col)
            normal, _, fg = COLORS[color]
            tested = self.rrect(x - 4, y - 4, x + KW + 4, y + KH + 4, 12, fill="", outline="", width=3)
            body = self.rrect(x, y, x + KW, y + KH, 9, fill=normal, outline="#111", width=1)
            led = c.create_oval(x + KW / 2 - 6, y + 6, x + KW / 2 + 6, y + 18, fill=LED_OFF, outline="#111")
            text = c.create_text(x + KW / 2, y + 38, text=label, fill=fg,
                                 font=("Segoe UI", 10, "bold"), justify="center")
            c.create_text(x + 5, y + 3, text=f"SW{sw}", anchor="nw",
                          font=("Segoe UI", 6), fill="#888")
            self.items[sw] = (tested, body, led, text, color)
            if sw == 5:
                self.chain_items[31] = (led, RED_OFF, RED_ON)
            if sw in SPECIAL:
                tag = SPECIAL[sw][0]
                bw = 7 + 6 * len(tag)
                self.rrect(x + KW - bw - 3, y + 3, x + KW - 3, y + 15, 4, fill=BADGE, outline="")
                c.create_text(x + KW - 3 - bw / 2, y + 9, text=tag, font=("Segoe UI", 7, "bold"), fill="#222")
            for item in (body, led, text):
                c.tag_bind(item, "<Button-1>", lambda e, n=sw: self.click(n))

        # Alphanumerisches Display (2 × HDSP-2112)
        dy = ROW_Y[2] + KH + 30
        self.rrect(LEFT + 40, dy, WIDTH - LEFT - 40, dy + 74, 10, fill="#111214", outline="#000")
        c.create_text(LEFT + 50, dy - 10, text="DIS2  ·  DIS1 (HDSP-2112)", anchor="w", font=("Segoe UI", 8),
                      fill="#6b6d69")
        self.bright = c.create_text(WIDTH - LEFT - 46, dy + 66, text="", anchor="e", font=("Segoe UI", 7),
                                    fill="#6b6d69")
        for col, (name, color) in STATUS.items():
            x = WIDTH - LEFT - 40 - (6 - col) * 48 - 24
            off, on = (GREEN_OFF, GREEN_ON) if color == "g" else (RED_OFF, RED_ON)
            item = c.create_oval(x, dy - 17, x + 10, dy - 7, fill=off, outline="#111")
            c.create_text(x + 13, dy - 12, text=name, anchor="w", font=("Segoe UI", 7), fill="#6b6d69")
            self.status_items[col] = (item, off, on)
        self.disp = c.create_text(WIDTH / 2, dy + 35, text="", fill="#ff5a2a", font=("Consolas", 26, "bold"))

        # Drehregler
        ky = ROW_Y[5] + KH + 26
        c.create_line(30, ky, WIDTH - 30, ky, fill="#a7a9a4", width=2)
        cx, cy, r = WIDTH / 2 - 40, ky + 82, 62
        c.create_oval(cx - r, cy - r, cx + r, cy + r, fill="#3b3d40", outline="#1b1c1d", width=3)
        c.create_oval(cx - r + 12, cy - r + 12, cx + r - 12, cy + r - 12, fill="#4a4d51", outline="")
        self.knob = (cx, cy)
        self.knob_dot = c.create_oval(0, 0, 0, 0, fill="#2a2c2e", outline="")
        self.knob_text = c.create_text(cx + r + 24, cy, anchor="w", text="Drehregler\n(Encoder)",
                                       font=("Segoe UI", 9), fill="#6b6d69")
        self.set_encoder(0)

    # ---------- Zustand ----------
    def set_key(self, sw, down):
        if sw not in self.items:
            return
        tested, body, led, text, color = self.items[sw]
        normal, lit, _ = COLORS[color]
        self.c.itemconfigure(body, fill=lit if down else normal)
        if down:
            self.c.itemconfigure(tested, outline=GLOW, width=6)
            self.tested.add(sw)
        else:
            self.c.itemconfigure(tested, outline=TESTED if sw in self.tested else "", width=3)

    def set_led(self, sw, on):
        (self.leds.add if on else self.leds.discard)(sw)
        if sw in self.items:
            self.c.itemconfigure(self.items[sw][2], fill=LED_ON if on else LED_OFF)

    def set_chain(self, k):
        for pos, (item, off, on) in self.chain_items.items():
            self.c.itemconfigure(item, fill=on if pos <= k else off)

    def set_encoder(self, n):
        """Griffmulde des Knopfs drehen: 1024 Schritte = eine Umdrehung."""
        cx, cy = self.knob
        a = math.radians(n * 360 / 1024 - 90)
        x, y = cx + 36 * math.cos(a), cy + 36 * math.sin(a)
        self.c.coords(self.knob_dot, x - 9, y - 9, x + 9, y + 9)
        self.c.itemconfigure(self.knob_text, text=f"Drehregler\nEncoder: {n}")

    def set_status(self, col, mode):
        """mode 0 = aus, 1 = an, 2 = blinkt (wie auf dem Board)."""
        self.status_mode[col] = mode
        self.draw_status(col)

    def draw_status(self, col):
        if col in self.status_items:
            item, c_off, c_on = self.status_items[col]
            mode = self.status_mode.get(col, 0)
            on = mode == 1 or (mode == 2 and self.blink_on)
            self.c.itemconfigure(item, fill=c_on if on else c_off)

    def blink(self):
        self.blink_on = not self.blink_on
        for col in self.status_items:
            if self.status_mode.get(col) == 2:
                self.draw_status(col)
        self.root.after(400, self.blink)

    def click(self, sw):
        if self.ser is not None:
            self.ser.write(f"K{sw}\n".encode())

    def reset_tested(self):
        self.tested = set(self.pressed)
        for sw in self.items:
            if sw not in self.pressed:
                self.c.itemconfigure(self.items[sw][0], outline="")
        self.update_counter()

    def update_counter(self):
        self.counter.configure(text=f"Getestet: {len(self.tested)} / {len(KEYS)}")

    def show_text(self, text):
        # 0x7F = Strichmännchen der SWEEP-Animation (selbstdefiniertes Zeichen auf dem Board)
        self.c.itemconfigure(self.disp, text=text.ljust(16)[:16].replace("\x7f", "♟"))

    def send_time(self):
        """Ortszeit an den Mega (für die Uhr auf DIFF. MODE)."""
        if self.ser is not None:
            self.ser.write(f"Z{calendar.timegm(time.localtime())}\n".encode())

    def time_tick(self):
        self.send_time()
        self.root.after(60000, self.time_tick)

    def send_text(self):
        if self.ser is not None:
            self.ser.write(f"W{self.entry.get()[:16]}\n".encode("utf-8"))   # Umlaute/Emojis setzt der Mega um

    # ---------- Serielle Schnittstelle ----------
    def reader(self, port):
        try:
            ser = serial.Serial(port, 115200, timeout=1)
            self.ser = ser
        except serial.SerialException as e:
            self.events.put(("err", str(e)))
            return
        while True:
            try:
                line = ser.readline().decode("utf-8", errors="replace").strip()
            except serial.SerialException as e:
                self.events.put(("err", str(e)))
                return
            if line:
                self.events.put(("line", line))

    def poll(self):
        while not self.events.empty():
            kind, data = self.events.get()
            if kind == "err":
                self.status.configure(text=f"Fehler: {data}", fg="#b00020")
            elif data == "KEYS READY":
                self.status.configure(text="Verbunden – Tasten drücken!", fg="#1b7f2a")
                self.send_time()
            elif data == "W" or data.startswith("W "):
                self.show_text(data[2:])
            elif data.startswith("H "):
                b = int(data[2:])
                self.c.itemconfigure(self.bright, text=f"Helligkeit {BRIGHT_PCT[b]} %")
            elif data.startswith("E "):
                self.set_encoder(int(data[2:]))
            elif data.startswith("C "):
                self.set_chain(int(data[2:]))
            elif data.startswith("S "):
                col, mode = data[2:].split()
                self.set_status(int(col), int(mode))
            elif data.startswith("L SW"):
                sw, on = data[4:].split()
                self.set_led(int(sw), on == "1")
            elif data[:4] in ("D SW", "U SW"):
                sw, down = int(data[4:]), data[0] == "D"
                (self.pressed.add if down else self.pressed.discard)(sw)
                self.set_key(sw, down)
                label = next((l for s, l, *_ in KEYS if s == sw), "?").replace("\n", " ")
                self.log.insert(0, f"{'▼' if down else '▲'} SW{sw:<3} {label}")
                self.log.delete(200, "end")
                self.update_counter()
        self.root.after(10, self.poll)


def main():
    if len(sys.argv) < 2:
        sys.exit("Aufruf: python tools\\keyboard_gui.py COM8")
    root = tk.Tk()
    KeyboardGUI(root, sys.argv[1])
    root.mainloop()


if __name__ == "__main__":
    main()
