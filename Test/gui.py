import tkinter as tk
import socket
import threading
import math
import re
import queue

class CybotGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Cybot Field Interface - CPRE 2880")
        self.root.geometry("1920x1080")
        
        self.msg_queue = queue.Queue()
        self.running = True
        self.sock = None
        
        # --- Bot State Variables ---
        self.canvas_width = 1800
        self.canvas_height = 600
        self.bot_x = self.canvas_width / 2
        self.bot_y = self.canvas_height - 40
        self.bot_heading = 90.0

        # Mapping scale: pixels per centimeter
        self.pixel_scale = 3.0

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

        # --- Movement Controls ---
        move_frame = tk.LabelFrame(self.root, text="Movement Controls", font=("Arial", 10, "bold"), padx=10, pady=0)
        move_frame.pack(pady=0, fill="x", padx=20)

        dist_frame = tk.Frame(move_frame)
        dist_frame.pack(pady=5)
        tk.Label(dist_frame, text="Drive (cm):").grid(row=0, column=0, rowspan=2, padx=5)
        
        tk.Button(dist_frame, text="↑ Fwd 10", width=10, command=lambda: self.send_command("q")).grid(row=0, column=1, padx=2, pady=2)
        tk.Button(dist_frame, text="↑ Fwd 25", width=10, command=lambda: self.send_command("w")).grid(row=0, column=2, padx=2, pady=2)
        tk.Button(dist_frame, text="↑ Fwd 50", width=10, command=lambda: self.send_command("e")).grid(row=0, column=3, padx=2, pady=2)
        
        tk.Button(dist_frame, text="↓ Rev 10", width=10, command=lambda: self.send_command("z")).grid(row=1, column=1, padx=2, pady=2)
        tk.Button(dist_frame, text="↓ Rev 25", width=10, command=lambda: self.send_command("x")).grid(row=1, column=2, padx=2, pady=2)
        tk.Button(dist_frame, text="↓ Rev 50", width=10, command=lambda: self.send_command("c")).grid(row=1, column=3, padx=2, pady=2)

        turn_frame = tk.Frame(move_frame)
        turn_frame.pack(pady=5)
        tk.Label(turn_frame, text="Turn (deg):").grid(row=0, column=0, rowspan=2, padx=5)
        
        tk.Button(turn_frame, text="← Left 15°", width=10, command=lambda: self.send_command("a")).grid(row=0, column=1, padx=2, pady=2)
        tk.Button(turn_frame, text="← Left 45°", width=10, command=lambda: self.send_command("s")).grid(row=0, column=2, padx=2, pady=2)
        tk.Button(turn_frame, text="← Left 90°", width=10, command=lambda: self.send_command("d")).grid(row=0, column=3, padx=2, pady=2)

        tk.Button(turn_frame, text="Right 15° →", width=10, command=lambda: self.send_command("f")).grid(row=1, column=1, padx=2, pady=2)
        tk.Button(turn_frame, text="Right 45° →", width=10, command=lambda: self.send_command("g")).grid(row=1, column=2, padx=2, pady=2)
        tk.Button(turn_frame, text="Right 90° →", width=10, command=lambda: self.send_command("h")).grid(row=1, column=3, padx=2, pady=2)

        control_frame = tk.Frame(move_frame)
        control_frame.pack(pady=5)
        tk.Button(control_frame, text="Deliver Pizza", width=22, font=("Arial", 10, "bold"), command=lambda: self.send_command("i")).grid(row=0, column=0, padx=2, pady=0)
        tk.Button(control_frame, text="Exit Program", width=22, bg="#ffcccc", font=("Arial", 10, "bold"), command=lambda: self.send_command("o")).grid(row=0, column=1, padx=2, pady=0)

        # --- Real-Time Data Label ---
        self.scan_label = tk.Label(self.root, text="Angle: ---  Distance: ---  Raw IR Value: ---", font=("Consolas", 14, "bold"))
        self.scan_label.pack(pady=0)

        # --- Map Controls ---
        map_ctrl_frame = tk.Frame(self.root)
        map_ctrl_frame.pack(pady=5)
        
        tk.Button(map_ctrl_frame, text="📡 Trigger Scan", width=22, bg="#ccffcc", font=("Arial", 10, "bold"), command=lambda: self.send_command("p")).grid(row=0, column=0, padx=10)
        tk.Button(map_ctrl_frame, text="Reset Map & Bot", width=22, command=self.clear_canvas, font=("Arial", 10)).grid(row=0, column=1, padx=10)

        # --- Field View (Canvas) ---
        self.canvas = tk.Canvas(self.root, width=self.canvas_width, height=self.canvas_height, bg="black")
        self.canvas.pack()
        
        self.clear_canvas() 

        # --- Event Log ---
        tk.Label(self.root, text="Event Log (Bumps, Cliffs, Tx/Rx)").pack(pady=(10, 0))
        self.log_text = tk.Text(self.root, height=8, width=80, state=tk.DISABLED, bg="#f0f0f0")
        self.log_text.pack(pady=5)

    def setup_key_bindings(self):
        self.root.bind("<w>", lambda event: self.send_command("w 10"))
        self.root.bind("<a>", lambda event: self.send_command("a 15"))
        self.root.bind("<s>", lambda event: self.send_command("s 10"))
        self.root.bind("<d>", lambda event: self.send_command("d 15"))
        self.root.bind("<space>", lambda event: self.send_command("x 0"))
        self.root.bind("<m>", lambda event: self.send_command("m 0")) # Bound to 'M' key

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
        self.bot_x = self.canvas_width / 2
        self.bot_y = self.canvas_height - 40
        self.bot_heading = 90.0
        self.canvas.delete("all")
        self.draw_scale()
        self.draw_bot()

    def draw_scale(self):
        # Draw a 50cm reference scale in the bottom left corner
        cm_length = 50
        pixel_length = cm_length * self.pixel_scale

        start_x = 15
        start_y = self.canvas_height - 20
        end_x = start_x + pixel_length
        end_y = start_y

        # Main horizontal line
        self.canvas.create_line(start_x, start_y, end_x, end_y, fill="white", width=2, tags="scale")
        # Left tick
        self.canvas.create_line(start_x, start_y-5, start_x, start_y+5, fill="white", width=2, tags="scale")
        # Right tick
        self.canvas.create_line(end_x, end_y-5, end_x, end_y+5, fill="white", width=2, tags="scale")
        # Label text
        self.canvas.create_text(start_x + (pixel_length/2), start_y - 12, text=f"{cm_length} cm", fill="white", font=("Arial", 9, "bold"), tags="scale")

    def draw_bot(self):
        self.canvas.delete("bot")
        cm_radius = 17
        r = cm_radius * self.pixel_scale
        self.canvas.create_oval(self.bot_x - r, self.bot_y - r, self.bot_x + r, self.bot_y + r, fill="blue", outline="white", tags="bot")
        
        rad = math.radians(self.bot_heading)
        arrow_length = 25
        end_x = self.bot_x + (arrow_length * math.cos(rad))
        end_y = self.bot_y - (arrow_length * math.sin(rad)) 
        
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

        scan_match = re.search(r"Angle:\s*(\d+).*?Distance:\s*([0-9.]+).*?Raw IR Value:\s*(\d+)", msg, re.IGNORECASE)
        if scan_match:
            self.scan_label.config(text=msg)
            angle = float(scan_match.group(1))
            distance = float(scan_match.group(2))
            ir_val = float(scan_match.group(3))
            ir_distance = (4727.5 * pow(ir_val, -0.829) / 10)

            distance += 17
            ir_distance += 17
            print(ir_distance)

            if ((distance - 17) < 80 and ir_distance < 30):
                self.draw_scan_point(angle, ir_distance)
            else:
                self.draw_scan_point(angle, distance)
            return 

        move_match = re.search(r"(forward|backward|moved?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if move_match:
            direction = move_match.group(1)
            amount = float(move_match.group(2))
            if "backward" in direction: amount = -amount
            if "mm" in lower_msg: amount /= 10.0
            self.update_bot_position(amount)

        turn_match = re.search(r"(left|right|turned?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if turn_match:
            direction = turn_match.group(1)
            amount = float(turn_match.group(2))
            if "right" in direction: amount = -amount 
            self.update_bot_heading(amount)

        if any(keyword in lower_msg for keyword in ["bump", "cliff", "move", "turn", "system", "error", "tx", "scan"]):
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
        self.bot_heading += angle_deg
        self.bot_heading %= 360
        self.draw_bot()

    def draw_scan_point(self, servo_angle, distance):
        pixel_dist = distance * self.pixel_scale
        world_angle = self.bot_heading + (servo_angle - 90)
        rad = math.radians(world_angle)
        x = self.bot_x + (pixel_dist * math.cos(rad))
        y = self.bot_y - (pixel_dist * math.sin(rad))

        if 0 <= x <= self.canvas_width and 0 <= y <= self.canvas_height:
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
