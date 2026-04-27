import tkinter as tk
import socket
import threading
import math
import re
import queue
from collections import deque

class CybotGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Cybot Field Interface - CPRE 2880")
        self.root.geometry("1920x1180")
        
        self.msg_queue = queue.Queue()
        self.running = True
        self.sock = None
        
        # --- Bot State Variables ---
        self.canvas_width = 1200
        self.canvas_height = 550
        self.bot_x = self.canvas_width / 2
        self.bot_y = self.canvas_height - 40
        self.bot_heading = 90.0

        # Mapping scale: pixels per centimeter
        self.pixel_scale = 3.0

        # --- IR History (last 60 readings) ---
        self.ir_history = deque(maxlen=60)
        self.IR_MIN_CM = 10.0
        self.IR_MAX_CM = 80.0

        self.setup_ui()
        self.setup_key_bindings()
        
        self.root.after(100, self.process_queue)

    # -------------------------------------------------------------------------
    #  UI SETUP
    # -------------------------------------------------------------------------
    def setup_ui(self):
        # === Top bar: connection ===
        conn_frame = tk.Frame(self.root)
        conn_frame.pack(pady=6)
        
        tk.Label(conn_frame, text="Cybot IP:").grid(row=0, column=0, padx=5)
        self.ip_entry = tk.Entry(conn_frame, width=15)
        self.ip_entry.insert(0, "192.168.1.1")
        self.ip_entry.grid(row=0, column=1, padx=5)
        
        self.connect_btn = tk.Button(conn_frame, text="Connect", command=self.connect_to_bot)
        self.connect_btn.grid(row=0, column=2, padx=5)

        # === Main horizontal layout: controls LEFT | map CENTER | IR panel RIGHT ===
        main_frame = tk.Frame(self.root)
        main_frame.pack(fill="both", expand=True, padx=10)

        # --- LEFT: movement controls ---
        left_frame = tk.Frame(main_frame, width=220)
        left_frame.pack(side=tk.LEFT, fill="y", padx=(0, 10))
        left_frame.pack_propagate(False)
        self._build_movement_controls(left_frame)

        # --- CENTER: field map + scan label + map controls ---
        center_frame = tk.Frame(main_frame)
        center_frame.pack(side=tk.LEFT, fill="both", expand=True)
        self._build_map_area(center_frame)

        # --- RIGHT: IR gauge + history chart ---
        right_frame = tk.Frame(main_frame, width=300, bg="#1a1a2e")
        right_frame.pack(side=tk.RIGHT, fill="y", padx=(10, 0))
        right_frame.pack_propagate(False)
        self._build_ir_panel(right_frame)

        # === Event log at bottom ===
        tk.Label(self.root, text="Event Log (Bumps, Cliffs, Tx/Rx)").pack(pady=(6, 0))
        self.log_text = tk.Text(self.root, height=7, width=120, state=tk.DISABLED, bg="#f0f0f0", font=("Consolas", 9))
        self.log_text.pack(pady=4, padx=10)

    def _build_movement_controls(self, parent):
        tk.Label(parent, text="Movement Controls", font=("Arial", 11, "bold")).pack(pady=(8, 4))

        # --- Drive forward ---
        fwd_lf = tk.LabelFrame(parent, text="Drive Forward", padx=5, pady=5)
        fwd_lf.pack(fill="x", pady=3)
        tk.Button(fwd_lf, text="↑ Fwd 10 cm", width=14, command=lambda: self.send_command("q")).pack(pady=2)
        tk.Button(fwd_lf, text="↑ Fwd 25 cm", width=14, command=lambda: self.send_command("w")).pack(pady=2)
        tk.Button(fwd_lf, text="↑ Fwd 50 cm", width=14, command=lambda: self.send_command("e")).pack(pady=2)

        # --- Drive reverse ---
        rev_lf = tk.LabelFrame(parent, text="Drive Reverse", padx=5, pady=5)
        rev_lf.pack(fill="x", pady=3)
        tk.Button(rev_lf, text="↓ Rev 10 cm", width=14, command=lambda: self.send_command("z")).pack(pady=2)
        tk.Button(rev_lf, text="↓ Rev 25 cm", width=14, command=lambda: self.send_command("x")).pack(pady=2)
        tk.Button(rev_lf, text="↓ Rev 50 cm", width=14, command=lambda: self.send_command("c")).pack(pady=2)

        # --- Turn left ---
        tl_lf = tk.LabelFrame(parent, text="Turn Left", padx=5, pady=5)
        tl_lf.pack(fill="x", pady=3)
        tk.Button(tl_lf, text="← Left  15°", width=14, command=lambda: self.send_command("a")).pack(pady=2)
        tk.Button(tl_lf, text="← Left  45°", width=14, command=lambda: self.send_command("s")).pack(pady=2)
        tk.Button(tl_lf, text="← Left  90°", width=14, command=lambda: self.send_command("d")).pack(pady=2)

        # --- Turn right ---
        tr_lf = tk.LabelFrame(parent, text="Turn Right", padx=5, pady=5)
        tr_lf.pack(fill="x", pady=3)
        tk.Button(tr_lf, text="Right 15° →", width=14, command=lambda: self.send_command("f")).pack(pady=2)
        tk.Button(tr_lf, text="Right 45° →", width=14, command=lambda: self.send_command("g")).pack(pady=2)
        tk.Button(tr_lf, text="Right 90° →", width=14, command=lambda: self.send_command("h")).pack(pady=2)

        # --- Special actions ---
        act_lf = tk.LabelFrame(parent, text="Actions", padx=5, pady=5)
        act_lf.pack(fill="x", pady=3)
        tk.Button(act_lf, text="🍕 Deliver Pizza", width=14, font=("Arial", 9, "bold"),
                  command=lambda: self.send_command("i")).pack(pady=2)
        tk.Button(act_lf, text="Exit Program", width=14, bg="#ffcccc", font=("Arial", 9, "bold"),
                  command=lambda: self.send_command("o")).pack(pady=2)

    def _build_map_area(self, parent):
        # Scan data label
        self.scan_label = tk.Label(
            parent,
            text="Angle: ---  |  Ping: --- cm  |  IR Raw: ---  |  IR Distance: --- cm",
            font=("Consolas", 12, "bold"), bg="#e8f4f8", relief="groove", pady=4
        )
        self.scan_label.pack(fill="x", pady=(4, 2))

        # Map controls row
        map_ctrl = tk.Frame(parent)
        map_ctrl.pack(pady=3)
        tk.Button(map_ctrl, text="📡 Trigger Scan", width=20, bg="#ccffcc",
                  font=("Arial", 10, "bold"), command=lambda: self.send_command("p")).grid(row=0, column=0, padx=8)
        tk.Button(map_ctrl, text="Reset Map & Bot", width=20,
                  font=("Arial", 10), command=self.clear_canvas).grid(row=0, column=1, padx=8)

        # Legend
        legend = tk.Frame(parent)
        legend.pack()
        tk.Label(legend, text="●", fg="red",   font=("Arial", 14)).grid(row=0, column=0, padx=2)
        tk.Label(legend, text="Ping (sonar)",  font=("Arial", 9)).grid(row=0, column=1, padx=4)
        tk.Label(legend, text="●", fg="#00ff88", font=("Arial", 14)).grid(row=0, column=2, padx=2)
        tk.Label(legend, text="IR distance",   font=("Arial", 9)).grid(row=0, column=3, padx=4)
        tk.Label(legend, text="●", fg="blue",  font=("Arial", 14)).grid(row=0, column=4, padx=2)
        tk.Label(legend, text="Bot",           font=("Arial", 9)).grid(row=0, column=5, padx=4)

        # Canvas
        self.canvas = tk.Canvas(parent, width=self.canvas_width, height=self.canvas_height, bg="black")
        self.canvas.pack()
        self.clear_canvas()

    def _build_ir_panel(self, parent):
        """Right panel: live IR bar gauge + scrolling history chart."""
        tk.Label(parent, text="IR Sensor Panel", font=("Arial", 12, "bold"),
                 bg="#1a1a2e", fg="white").pack(pady=(10, 4))

        # ---- Live bar gauge ----
        tk.Label(parent, text="Live Distance", font=("Arial", 10),
                 bg="#1a1a2e", fg="#aaaaaa").pack()

        gauge_frame = tk.Frame(parent, bg="#1a1a2e")
        gauge_frame.pack(pady=4)

        self.gauge_canvas = tk.Canvas(gauge_frame, width=60, height=300,
                                      bg="#0d0d1a", highlightthickness=1,
                                      highlightbackground="#444")
        self.gauge_canvas.pack(side=tk.LEFT, padx=(8, 4))

        # Axis labels (80 cm at top, 10 cm at bottom = valid range)
        axis_frame = tk.Frame(gauge_frame, bg="#1a1a2e")
        axis_frame.pack(side=tk.LEFT, anchor="n", pady=0)
        for label in ["10 cm", "20", "30", "40", "50", "60", "70", "80 cm"]:
            tk.Label(axis_frame, text=label, font=("Arial", 7),
                     bg="#1a1a2e", fg="#888888").pack()

        self.ir_distance_label = tk.Label(parent, text="--- cm",
                                          font=("Consolas", 18, "bold"),
                                          bg="#1a1a2e", fg="#00ff88")
        self.ir_distance_label.pack(pady=4)

        self.ir_raw_label = tk.Label(parent, text="Raw: ---",
                                     font=("Consolas", 11),
                                     bg="#1a1a2e", fg="#aaaaaa")
        self.ir_raw_label.pack()

        # ---- History chart ----
        tk.Label(parent, text="IR History (last 60)", font=("Arial", 10),
                 bg="#1a1a2e", fg="#aaaaaa").pack(pady=(14, 2))

        self.history_canvas = tk.Canvas(parent, width=270, height=180,
                                        bg="#0d0d1a", highlightthickness=1,
                                        highlightbackground="#444")
        self.history_canvas.pack(padx=10)
        self._draw_history_axes()

        # ---- In-range indicator ----
        self.range_label = tk.Label(parent, text="⬤ In Range",
                                    font=("Arial", 10, "bold"),
                                    bg="#1a1a2e", fg="#555555")
        self.range_label.pack(pady=6)

    # -------------------------------------------------------------------------
    #  IR SENSOR UTILITIES
    # -------------------------------------------------------------------------
    def convert_raw_to_cm(self, raw_val):
        """GP2D12 conversion: distance_cm = 6e6 * raw^(-1.692)"""
        if raw_val <= 0:
            return None
        dist = (6e6) * (raw_val ** -1.692)
        return dist

    def update_ir_display(self, raw_val, dist_cm):
        """Refresh the right-panel gauge, labels, and history chart."""
        # Clamp for display purposes
        in_range = (dist_cm is not None and self.IR_MIN_CM <= dist_cm <= self.IR_MAX_CM)

        # Distance label
        if dist_cm is not None:
            self.ir_distance_label.config(
                text=f"{dist_cm:.1f} cm",
                fg="#00ff88" if in_range else "#ff6644"
            )
        else:
            self.ir_distance_label.config(text="--- cm", fg="#555555")

        # Raw label
        self.ir_raw_label.config(text=f"Raw: {raw_val}")

        # Range indicator
        if dist_cm is None:
            self.range_label.config(text="⬤  No Reading", fg="#555555")
        elif in_range:
            self.range_label.config(text="⬤  In Range (10–80 cm)", fg="#00ff88")
        else:
            self.range_label.config(text="⬤  Out of Range", fg="#ff6644")

        # Gauge bar (gauge_canvas: height=300, maps 10cm→bottom, 80cm→top)
        self.gauge_canvas.delete("all")
        gc_h = 300
        gc_w = 60
        # Background track
        self.gauge_canvas.create_rectangle(10, 5, gc_w - 10, gc_h - 5,
                                           fill="#1a1a2e", outline="#333")
        if dist_cm is not None:
            clamped = max(self.IR_MIN_CM, min(self.IR_MAX_CM, dist_cm))
            # 10cm = bottom (gc_h-5), 80cm = top (5)
            frac = (clamped - self.IR_MIN_CM) / (self.IR_MAX_CM - self.IR_MIN_CM)
            bar_top = gc_h - 5 - frac * (gc_h - 10)
            bar_color = "#00ff88" if in_range else "#ff6644"
            self.gauge_canvas.create_rectangle(10, bar_top, gc_w - 10, gc_h - 5,
                                               fill=bar_color, outline="")
            # Tick line
            self.gauge_canvas.create_line(5, bar_top, gc_w - 5, bar_top,
                                          fill="white", width=2)

        # History
        if dist_cm is not None:
            self.ir_history.append(dist_cm)
        self._draw_ir_history()

    def _draw_history_axes(self):
        """Draw static axis grid on history canvas."""
        w, h = 270, 180
        pad_l, pad_b = 30, 20
        plot_w = w - pad_l - 8
        plot_h = h - pad_b - 8

        self.history_canvas.create_rectangle(pad_l, 8, w - 8, h - pad_b,
                                              outline="#333", fill="#0d0d1a")
        # Horizontal grid lines at 10, 30, 50, 70 cm
        for cm in [10, 30, 50, 70, 80]:
            frac = (cm - self.IR_MIN_CM) / (self.IR_MAX_CM - self.IR_MIN_CM)
            y = (h - pad_b) - frac * plot_h
            self.history_canvas.create_line(pad_l, y, w - 8, y,
                                            fill="#222", dash=(2, 4))
            self.history_canvas.create_text(pad_l - 3, y, text=str(cm),
                                            font=("Arial", 6), fill="#666", anchor="e")
        # X label
        self.history_canvas.create_text(w // 2, h - 6, text="← older    newer →",
                                        font=("Arial", 7), fill="#555")

    def _draw_ir_history(self):
        w, h = 270, 180
        pad_l, pad_b = 30, 20
        plot_w = w - pad_l - 8
        plot_h = h - pad_b - 8

        self.history_canvas.delete("data")
        self._draw_history_axes()

        hist = list(self.ir_history)
        if len(hist) < 2:
            return

        max_pts = 60
        pts = hist[-max_pts:]
        n = len(pts)

        coords = []
        for i, val in enumerate(pts):
            frac = (max(self.IR_MIN_CM, min(self.IR_MAX_CM, val)) - self.IR_MIN_CM) / \
                   (self.IR_MAX_CM - self.IR_MIN_CM)
            x = pad_l + (i / (max_pts - 1)) * plot_w
            y = (h - pad_b) - frac * plot_h
            coords.extend([x, y])

        if len(coords) >= 4:
            self.history_canvas.create_line(*coords, fill="#00ff88", width=2,
                                            smooth=True, tags="data")
        # Draw dots for last point
        lx, ly = coords[-2], coords[-1]
        self.history_canvas.create_oval(lx - 3, ly - 3, lx + 3, ly + 3,
                                        fill="white", outline="", tags="data")

    # -------------------------------------------------------------------------
    #  MAP / CANVAS
    # -------------------------------------------------------------------------
    def clear_canvas(self):
        self.bot_x = self.canvas_width / 2
        self.bot_y = self.canvas_height - 40
        self.bot_heading = 90.0
        self.canvas.delete("all")
        self.draw_grid()
        self.draw_scale()
        self.draw_bot()

    def draw_grid(self):
        """Faint 50cm grid lines."""
        grid_px = 50 * self.pixel_scale  # 50 cm in pixels
        for x in range(0, self.canvas_width, int(grid_px)):
            self.canvas.create_line(x, 0, x, self.canvas_height,
                                    fill="#1a1a1a", tags="grid")
        for y in range(0, self.canvas_height, int(grid_px)):
            self.canvas.create_line(0, y, self.canvas_width, y,
                                    fill="#1a1a1a", tags="grid")

    def draw_scale(self):
        cm_length = 50
        pixel_length = cm_length * self.pixel_scale
        sx, sy = 15, self.canvas_height - 20
        ex = sx + pixel_length
        self.canvas.create_line(sx, sy, ex, sy, fill="white", width=2, tags="scale")
        self.canvas.create_line(sx, sy - 5, sx, sy + 5, fill="white", width=2, tags="scale")
        self.canvas.create_line(ex, sy - 5, ex, sy + 5, fill="white", width=2, tags="scale")
        self.canvas.create_text(sx + pixel_length / 2, sy - 12,
                                text=f"{cm_length} cm", fill="white",
                                font=("Arial", 9, "bold"), tags="scale")

    def draw_bot(self):
        self.canvas.delete("bot")
        r = 15
        self.canvas.create_oval(self.bot_x - r, self.bot_y - r,
                                 self.bot_x + r, self.bot_y + r,
                                 fill="blue", outline="white", tags="bot")
        rad = math.radians(self.bot_heading)
        ex = self.bot_x + 25 * math.cos(rad)
        ey = self.bot_y - 25 * math.sin(rad)
        self.canvas.create_line(self.bot_x, self.bot_y, ex, ey,
                                 fill="yellow", arrow=tk.LAST, width=2, tags="bot")
        self.canvas.create_text(self.bot_x, self.bot_y + 20,
                                 text="BOT", fill="white", font=("Arial", 8), tags="bot")

    def draw_ping_point(self, servo_angle, distance_cm):
        """Red dot — ping/sonar reading."""
        self._draw_sensor_point(servo_angle, distance_cm, color="red")

    def draw_ir_point(self, servo_angle, distance_cm):
        """Green dot — IR reading."""
        if distance_cm is None or not (self.IR_MIN_CM <= distance_cm <= self.IR_MAX_CM):
            return  # Don't plot out-of-range IR on map
        self._draw_sensor_point(servo_angle, distance_cm, color="#00ff88")

    def _draw_sensor_point(self, servo_angle, distance_cm, color):
        pixel_dist = distance_cm * self.pixel_scale
        world_angle = self.bot_heading + (servo_angle - 90)
        rad = math.radians(world_angle)
        x = self.bot_x + pixel_dist * math.cos(rad)
        y = self.bot_y - pixel_dist * math.sin(rad)
        if 0 <= x <= self.canvas_width and 0 <= y <= self.canvas_height:
            self.canvas.create_oval(x - 3, y - 3, x + 3, y + 3,
                                     fill=color, outline=color)

    # -------------------------------------------------------------------------
    #  NETWORKING
    # -------------------------------------------------------------------------
    def connect_to_bot(self):
        ip = self.ip_entry.get()
        port = 288
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(3.0)
            self.sock.connect((ip, port))
            self.sock.settimeout(None)
            self.log_event(f"[SYSTEM] Connected to {ip}:{port}")
            self.connect_btn.config(state=tk.DISABLED)
            threading.Thread(target=self.receive_data, daemon=True).start()
        except Exception as e:
            self.log_event(f"[ERROR] Connection failed: {e}")

    def receive_data(self):
        buffer = ""
        while self.running:
            try:
                data = self.sock.recv(1024).decode('utf-8', errors='ignore')
                if not data:
                    self.msg_queue.put("[SYSTEM] Connection closed by Cybot.")
                    break
                buffer += data
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    self.msg_queue.put(line.strip())
            except Exception as e:
                self.msg_queue.put(f"[SYSTEM] Disconnected: {e}")
                break
        self.connect_btn.config(state=tk.NORMAL)
        self.sock = None

    def send_command(self, cmd_string):
        if self.sock:
            try:
                self.sock.sendall(f"{cmd_string}\n".encode('utf-8'))
                self.log_event(f"[TX] Sent command: '{cmd_string}'")
            except Exception as e:
                self.log_event(f"[ERROR] Failed to send: {e}")
        else:
            self.log_event(f"[SYSTEM] Cannot send '{cmd_string}'. Not connected.")

    # -------------------------------------------------------------------------
    #  MESSAGE PARSING
    # -------------------------------------------------------------------------
    def process_queue(self):
        while not self.msg_queue.empty():
            self.parse_message(self.msg_queue.get())
        self.root.after(50, self.process_queue)

    def parse_message(self, msg):
        if not msg:
            return
        lower_msg = msg.lower()

        # --- Scan data: "Angle: X  Distance: Y  Raw IR Value: Z" ---
        scan_match = re.search(
            r"Angle:\s*(\d+).*?Distance:\s*([0-9.]+).*?Raw IR Value:\s*(\d+)",
            msg, re.IGNORECASE
        )
        if scan_match:
            angle    = float(scan_match.group(1))
            ping_cm  = float(scan_match.group(2))
            ir_raw   = int(scan_match.group(3))

            ir_cm = self.convert_raw_to_cm(ir_raw)

            # Update scan label
            ir_text = f"{ir_cm:.1f}" if ir_cm is not None else "---"
            self.scan_label.config(
                text=(f"Angle: {angle:.0f}°  |  Ping: {ping_cm:.1f} cm  |"
                      f"  IR Raw: {ir_raw}  |  IR Distance: {ir_text} cm")
            )

            # Plot both sensors on the map
            self.draw_ping_point(angle, ping_cm)
            self.draw_ir_point(angle, ir_cm)

            # Update right-panel IR display
            self.update_ir_display(ir_raw, ir_cm)
            return

        # --- Movement feedback ---
        move_match = re.search(r"(forward|backward|moved?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if move_match:
            direction = move_match.group(1)
            amount = float(move_match.group(2))
            if "backward" in direction:
                amount = -amount
            if "mm" in lower_msg:
                amount /= 10.0
            self.update_bot_position(amount)

        turn_match = re.search(r"(left|right|turned?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if turn_match:
            direction = turn_match.group(1)
            amount = float(turn_match.group(2))
            if "right" in direction:
                amount = -amount
            self.update_bot_heading(amount)

        if any(kw in lower_msg for kw in ["bump", "cliff", "move", "turn", "system", "error", "tx", "scan"]):
            self.log_event(msg)
        else:
            self.log_event(f"[RX] {msg}")

    def update_bot_position(self, distance_cm):
        pixel_dist = distance_cm * self.pixel_scale
        rad = math.radians(self.bot_heading)
        self.bot_x += pixel_dist * math.cos(rad)
        self.bot_y -= pixel_dist * math.sin(rad)
        self.draw_bot()

    def update_bot_heading(self, angle_deg):
        self.bot_heading = (self.bot_heading + angle_deg) % 360
        self.draw_bot()

    # -------------------------------------------------------------------------
    #  KEY BINDINGS
    # -------------------------------------------------------------------------
    def setup_key_bindings(self):
        self.root.bind("<w>", lambda e: self.send_command("w"))
        self.root.bind("<a>", lambda e: self.send_command("a"))
        self.root.bind("<s>", lambda e: self.send_command("s"))
        self.root.bind("<d>", lambda e: self.send_command("d"))
        self.root.bind("<space>", lambda e: self.send_command("x 0"))
        self.root.bind("<m>", lambda e: self.send_command("m 0"))

    # -------------------------------------------------------------------------
    #  LOGGING
    # -------------------------------------------------------------------------
    def log_event(self, text):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, text + "\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)

    def on_closing(self):
        self.running = False
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = CybotGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
