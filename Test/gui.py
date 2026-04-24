import tkinter as tk
import socket
import threading
import math
import re
import queue

class CybotGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Cybot Field Interface - CPRE 288")
        self.root.geometry("700x950") # Expanded to fit a larger map
        
        self.msg_queue = queue.Queue()
        self.running = True
        self.sock = None
        
        # --- Bot State Variables ---
        self.canvas_width = 600
        self.canvas_height = 400
        self.bot_x = self.canvas_width / 2
        self.bot_y = self.canvas_height - 40
        self.bot_heading = 90.0 # 90 degrees is 'Up' on the screen

        self.setup_ui()
        self.setup_key_bindings()
        
        self.root.after(100, self.process_queue)

    def setup_ui(self):
        # --- Connection Frame ---
        conn_frame = tk.Frame(self.root)
        conn_frame.pack(pady=10)
        
        tk.Label(conn_frame, text="Cybot IP:").grid(row=0, column=0, padx=5)
        self.ip_entry = tk.Entry(conn_frame, width=15)
        self.ip_entry.insert(0, "192.168.1.1") 
        self.ip_entry.grid(row=0, column=1, padx=5)
        
        self.connect_btn = tk.Button(conn_frame, text="Connect", command=self.connect_to_bot)
        self.connect_btn.grid(row=0, column=2, padx=5)

        # --- Advanced Movement Controls ---
        move_frame = tk.LabelFrame(self.root, text="Movement Controls", font=("Arial", 10, "bold"), padx=10, pady=10)
        move_frame.pack(pady=5, fill="x", padx=20)

        dist_frame = tk.Frame(move_frame)
        dist_frame.pack(pady=5)
        tk.Label(dist_frame, text="Drive (cm):").grid(row=0, column=0, rowspan=2, padx=5)
        
        tk.Button(dist_frame, text="↑ Fwd 10", width=10, command=lambda: self.send_command("w 10")).grid(row=0, column=1, padx=2, pady=2)
        tk.Button(dist_frame, text="↑ Fwd 25", width=10, command=lambda: self.send_command("w 25")).grid(row=0, column=2, padx=2, pady=2)
        tk.Button(dist_frame, text="↑ Fwd 50", width=10, command=lambda: self.send_command("w 50")).grid(row=0, column=3, padx=2, pady=2)
        
        tk.Button(dist_frame, text="↓ Rev 10", width=10, command=lambda: self.send_command("s 10")).grid(row=1, column=1, padx=2, pady=2)
        tk.Button(dist_frame, text="↓ Rev 25", width=10, command=lambda: self.send_command("s 25")).grid(row=1, column=2, padx=2, pady=2)
        tk.Button(dist_frame, text="↓ Rev 50", width=10, command=lambda: self.send_command("s 50")).grid(row=1, column=3, padx=2, pady=2)

        turn_frame = tk.Frame(move_frame)
        turn_frame.pack(pady=5)
        tk.Label(turn_frame, text="Turn (deg):").grid(row=0, column=0, rowspan=2, padx=5)
        
        tk.Button(turn_frame, text="← Left 15°", width=10, command=lambda: self.send_command("a 15")).grid(row=0, column=1, padx=2, pady=2)
        tk.Button(turn_frame, text="← Left 45°", width=10, command=lambda: self.send_command("a 45")).grid(row=0, column=2, padx=2, pady=2)
        tk.Button(turn_frame, text="← Left 90°", width=10, command=lambda: self.send_command("a 90")).grid(row=0, column=3, padx=2, pady=2)

        tk.Button(turn_frame, text="Right 15° →", width=10, command=lambda: self.send_command("d 15")).grid(row=1, column=1, padx=2, pady=2)
        tk.Button(turn_frame, text="Right 45° →", width=10, command=lambda: self.send_command("d 45")).grid(row=1, column=2, padx=2, pady=2)
        tk.Button(turn_frame, text="Right 90° →", width=10, command=lambda: self.send_command("d 90")).grid(row=1, column=3, padx=2, pady=2)

        tk.Button(move_frame, text="■ EMERGENCY STOP (Space)", width=30, bg="#ffcccc", font=("Arial", 10, "bold"), command=lambda: self.send_command("x 0")).pack(pady=10)

        # --- Real-Time Data Label ---
        self.scan_label = tk.Label(self.root, text="Angle: ---  Distance: ---  Raw IR Value: ---", font=("Consolas", 14, "bold"))
        self.scan_label.pack(pady=10)

        # --- Field View (Canvas) ---
        tk.Label(self.root, text="Dynamic Map (Bot & Scans)").pack()
        self.canvas = tk.Canvas(self.root, width=self.canvas_width, height=self.canvas_height, bg="black")
        self.canvas.pack()
        
        tk.Button(self.root, text="Reset Map & Bot", command=self.clear_canvas).pack(pady=5)
        self.clear_canvas() 

        # --- Event Log ---
        tk.Label(self.root, text="Event Log (Bumps, Cliffs, Tx/Rx)").pack()
        self.log_text = tk.Text(self.root, height=8, width=80, state=tk.DISABLED, bg="#f0f0f0")
        self.log_text.pack(pady=5)

    def setup_key_bindings(self):
        self.root.bind("<w>", lambda event: self.send_command("w 10"))
        self.root.bind("<a>", lambda event: self.send_command("a 15"))
        self.root.bind("<s>", lambda event: self.send_command("s 10"))
        self.root.bind("<d>", lambda event: self.send_command("d 15"))
        self.root.bind("<space>", lambda event: self.send_command("x 0"))

    def send_command(self, cmd_string):
        if self.sock:
            try:
                message = f"{cmd_string}\n"
                self.sock.sendall(message.encode('utf-8'))
                self.log_event(f"[TX] Sent command: '{cmd_string}'")
            except Exception as e:
                self.log_event(f"[ERROR] Failed to send: {e}")
        else:
            self.log_event(f"[SYSTEM] Cannot send '{cmd_string}'. Not connected.")

    def clear_canvas(self):
        # Reset bot back to starting location
        self.bot_x = self.canvas_width / 2
        self.bot_y = self.canvas_height - 40
        self.bot_heading = 90.0
        self.canvas.delete("all")
        self.draw_bot()

    def draw_bot(self):
        # Clear ONLY the old bot drawings, leave the map points alone
        self.canvas.delete("bot")
        
        # 1. Draw bot body
        r = 15 # radius
        self.canvas.create_oval(self.bot_x - r, self.bot_y - r, self.bot_x + r, self.bot_y + r, fill="blue", outline="white", tags="bot")
        
        # 2. Draw Heading Arrow
        # Calculate endpoint of the arrow based on current heading
        rad = math.radians(self.bot_heading)
        arrow_length = 25
        end_x = self.bot_x + (arrow_length * math.cos(rad))
        end_y = self.bot_y - (arrow_length * math.sin(rad)) # Y is inverted in Tkinter
        
        self.canvas.create_line(self.bot_x, self.bot_y, end_x, end_y, fill="yellow", arrow=tk.LAST, width=2, tags="bot")
        self.canvas.create_text(self.bot_x, self.bot_y + 20, text="BOT", fill="white", font=("Arial", 8), tags="bot")

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

    def process_queue(self):
        while not self.msg_queue.empty():
            msg = self.msg_queue.get()
            self.parse_message(msg)
        self.root.after(50, self.process_queue)

    def parse_message(self, msg):
        if not msg: return
        lower_msg = msg.lower()

        # --- 1. Parse Scan Data ---
        scan_match = re.search(r"Angle:\s*(\d+).*?Distance:\s*([0-9.]+).*?Raw IR Value:\s*(\d+)", msg, re.IGNORECASE)
        if scan_match:
            self.scan_label.config(text=msg)
            angle = float(scan_match.group(1))
            distance = float(scan_match.group(2))
            self.draw_scan_point(angle, distance)
            return # Don't log spammy scan data

        # --- 2. Parse Bot Movement ---
        # Looks for "moved 10", "moved 10mm", "forward 15", "backward 5"
        move_match = re.search(r"(forward|backward|moved?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if move_match:
            direction = move_match.group(1)
            amount = float(move_match.group(2))
            
            if "backward" in direction:
                amount = -amount # Reverse
                
            # Assume GUI scale runs in cm. If bot reports mm, convert it.
            if "mm" in lower_msg:
                amount /= 10.0
                
            self.update_bot_position(amount)

        # --- 3. Parse Bot Turning ---
        # Looks for "turned 5 degrees", "left 15", "right 90"
        turn_match = re.search(r"(left|right|turned?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if turn_match:
            direction = turn_match.group(1)
            amount = float(turn_match.group(2))
            
            # Math angles: Left (counter-clockwise) is positive, Right is negative
            if "right" in direction:
                amount = -amount 
                
            self.update_bot_heading(amount)

        # Log all non-scan events
        if any(keyword in lower_msg for keyword in ["bump", "cliff", "move", "turn", "system", "error", "tx"]):
            self.log_event(msg)
        else:
            self.log_event(f"[RX] {msg}")

    def update_bot_position(self, distance_cm):
        scale = 3.0 # Pixels per cm
        pixel_dist = distance_cm * scale
        
        # Calculate new X/Y using trigonometry based on current heading
        rad = math.radians(self.bot_heading)
        self.bot_x += pixel_dist * math.cos(rad)
        self.bot_y -= pixel_dist * math.sin(rad) # Y goes down in Tkinter
        
        self.draw_bot()

    def update_bot_heading(self, angle_deg):
        self.bot_heading += angle_deg
        self.bot_heading %= 360 # Keep within 0-359 degrees
        self.draw_bot()

    def draw_scan_point(self, servo_angle, distance):
        scale = 3.0 
        pixel_dist = distance * scale

        # Calculate object's REAL angle in the world
        # servo_angle assumes 90 is straight ahead relative to the bot.
        # So we add the bot's heading, plus the offset of the servo.
        world_angle = self.bot_heading + (servo_angle - 90)
        rad = math.radians(world_angle)
        
        # Plot point relative to bot's current X/Y
        x = self.bot_x + (pixel_dist * math.cos(rad))
        y = self.bot_y - (pixel_dist * math.sin(rad))

        if 0 <= x <= self.canvas_width and 0 <= y <= self.canvas_height:
            # We don't use tags="bot" here, so these points stay permanently like a map!
            self.canvas.create_oval(x-2, y-2, x+2, y+2, fill="red", outline="red")

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