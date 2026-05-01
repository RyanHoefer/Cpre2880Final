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
        
        # --- GUI Viewport Variables ---
        self.canvas_width = 600
        self.canvas_height = 400
        self.view_cx = 0.0 # Camera center X (in world cm)
        self.view_cy = 20.0 # Camera center Y (in world cm)
        self.zoom = 3.0 # Pixels per cm
        self.needs_redraw = False
        
        # --- Bot State & Mapping (World Coordinates in cm) ---
        self.bot_world_x = 0.0
        self.bot_world_y = 0.0
        self.bot_heading = 90.0 # 90 is North/Up
        
        # Cybot physical dimensions
        self.bot_radius_cm = 33.0 / 2.0 
        self.sensor_offset_cm = 8.0 # Distance from center of bot to the IR/Ping sensors
        
        # Data storage for the infinite map
        self.path_points = [(0.0, 0.0)]
        self.scan_points = []

        # Hazard Storage
        self.cliff_points = []
        self.boundary_points = []
        self.bump_points = []

        # --- Sensor Fusion Variables ---
        self.tracking_object = False
        self.prev_ir_value = 0

        self.setup_ui()
        self.setup_bindings()
        
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
        tk.Button(map_ctrl_frame, text="Reset Map & Bot", width=22, command=self.reset_map_data, font=("Arial", 10)).grid(row=0, column=1, padx=10)

        # New slider for tuning the IR edge detection threshold on the fly
        tk.Label(map_ctrl_frame, text="IR Edge Threshold (Δ):", font=("Arial", 9, "bold")).grid(row=0, column=2, padx=(15, 0))
        self.ir_threshold_slider = tk.Scale(map_ctrl_frame, from_=50, to_=800, orient=tk.HORIZONTAL, length=150)
        self.ir_threshold_slider.set(200) # Default starting threshold
        self.ir_threshold_slider.grid(row=0, column=3, padx=5)

        # --- Field View (Canvas) ---
        tk.Label(self.root, text="Map: Click & Drag to Pan | Scroll to Zoom", font=("Arial", 9, "italic")).pack()
        self.canvas = tk.Canvas(self.root, width=self.canvas_width, height=self.canvas_height, bg="#111111", cursor="crosshair")
        self.canvas.pack()
        
        self.reset_map_data()

        # --- Event Log ---
        tk.Label(self.root, text="Event Log").pack(pady=(10, 0))
        self.log_text = tk.Text(self.root, height=8, width=80, state=tk.DISABLED, bg="#f0f0f0")
        self.log_text.pack(pady=5)

    def setup_bindings(self):
        self.root.bind("<w>", lambda event: self.send_command("q"))
        self.root.bind("<a>", lambda event: self.send_command("a"))
        self.root.bind("<s>", lambda event: self.send_command("z"))
        self.root.bind("<d>", lambda event: self.send_command("f"))
        self.root.bind("<space>", lambda event: self.send_command("p"))
        #self.root.bind("<m>", lambda event: self.send_command("m 0")) # Bound to 'M' key

        # Mouse Pan and Zoom bindings
        self.canvas.bind("<ButtonPress-1>", self.on_pan_start)
        self.canvas.bind("<B1-Motion>", self.on_pan_drag)
        # Windows/macOS Scroll
        self.canvas.bind("<MouseWheel>", self.on_zoom)
        # Linux Scroll
        self.canvas.bind("<Button-4>", self.on_zoom)
        self.canvas.bind("<Button-5>", self.on_zoom)

        # --- Viewport & Rendering Functions ---
    def on_pan_start(self, event):
        self.pan_start_x = event.x
        self.pan_start_y = event.y

    def on_pan_drag(self, event):
        dx = event.x - self.pan_start_x
        dy = event.y - self.pan_start_y
        # Convert screen pixels moved to world coordinates moved
        self.view_cx -= dx / self.zoom
        self.view_cy += dy / self.zoom # +dy because screen Y is inverted
        self.pan_start_x = event.x
        self.pan_start_y = event.y
        self.queue_redraw()

    def on_zoom(self, event):
        # Determine scroll direction (cross-platform)
        zoom_in = False
        if event.num == 4 or (hasattr(event, 'delta') and event.delta > 0):
            zoom_in = True
        elif event.num == 5 or (hasattr(event, 'delta') and event.delta < 0):
            zoom_in = False

        if zoom_in:
            self.zoom *= 1.1
        else:
            self.zoom /= 1.1

        # Limit zoom levels
        self.zoom = max(0.5, min(self.zoom, 15.0))
        self.queue_redraw()

    def world_to_screen(self, wx, wy):
        """Converts real-world cm to screen pixel coordinates, forcing Integers to prevent Tkinter crashes."""
        sx = (self.canvas_width / 2) + (wx - self.view_cx) * self.zoom
        sy = (self.canvas_height / 2) - (wy - self.view_cy) * self.zoom 
        return int(sx), int(sy)

    def queue_redraw(self):
        self.needs_redraw = True

    def perform_redraw(self):
        """Erases and rebuilds the canvas entirely from the stored world data"""
        self.canvas.delete("all")
        
        # 1. Draw Grid/Scale Reference (Fixed to screen)
        cm_length = 50
        pixel_length = cm_length * self.zoom
        self.canvas.create_line(15, self.canvas_height-20, 15+pixel_length, self.canvas_height-20, fill="#555555", width=2)
        self.canvas.create_text(15 + (pixel_length/2), self.canvas_height-30, text=f"{cm_length} cm", fill="#555555", font=("Arial", 8))

        # 2. Draw Trajectory Path
        if len(self.path_points) > 1:
            screen_coords = []
            for wx, wy in self.path_points:
                sx, sy = self.world_to_screen(wx, wy)
                screen_coords.extend([sx, sy])
            self.canvas.create_line(*screen_coords, fill="white", width=2, dash=(4, 2))

        # 3. Draw Scan Points
        for wx, wy in self.scan_points:
            sx, sy = self.world_to_screen(wx, wy)
            # Only draw if visible on screen to save rendering time
            if -10 <= sx <= self.canvas_width+10 and -10 <= sy <= self.canvas_height+10:
                self.canvas.create_oval(sx-2, sy-2, sx+2, sy+2, fill="#00ff00", outline="#00ff00")

        # 4. Draw Boundaries (White)
        for wx, wy in self.boundary_points:
            sx, sy = self.world_to_screen(wx, wy)
            if -10 <= sx <= self.canvas_width+10 and -10 <= sy <= self.canvas_height+10:
                self.canvas.create_oval(sx-4, sy-4, sx+4, sy+4, fill="white", outline="white")

        # 5. Draw Cliffs (Blue)
        for wx, wy in self.cliff_points:
            sx, sy = self.world_to_screen(wx, wy)
            if -10 <= sx <= self.canvas_width+10 and -10 <= sy <= self.canvas_height+10:
                self.canvas.create_oval(sx-4, sy-4, sx+4, sy+4, fill="#0088ff", outline="#0088ff")

        # 6. Draw Bumps (Red)
        for wx, wy in self.bump_points:
            sx, sy = self.world_to_screen(wx, wy)
            if -10 <= sx <= self.canvas_width+10 and -10 <= sy <= self.canvas_height+10:
                self.canvas.create_oval(sx-4, sy-4, sx+4, sy+4, fill="#ff0000", outline="#ff0000")

        # 7. Draw Bot Body
        bot_sx, bot_sy = self.world_to_screen(self.bot_world_x, self.bot_world_y)
        r_pixels = self.bot_radius_cm * self.zoom
        self.canvas.create_oval(bot_sx - r_pixels, bot_sy - r_pixels, bot_sx + r_pixels, bot_sy + r_pixels, fill="blue", outline="white")
        
        # 8. Draw Scanner Origin Point (Front of bot)
        rad_heading = math.radians(self.bot_heading)
        scan_wx = self.bot_world_x + self.sensor_offset_cm * math.cos(rad_heading)
        scan_wy = self.bot_world_y + self.sensor_offset_cm * math.sin(rad_heading)
        scan_sx, scan_sy = self.world_to_screen(scan_wx, scan_wy)
        self.canvas.create_oval(scan_sx-3, scan_sy-3, scan_sx+3, scan_sy+3, fill="red", outline="white")

        # 9. Draw Heading Arrow
        end_wx = self.bot_world_x + (20 * math.cos(rad_heading))
        end_wy = self.bot_world_y + (20 * math.sin(rad_heading))
        end_sx, end_sy = self.world_to_screen(end_wx, end_wy)
        self.canvas.create_line(bot_sx, bot_sy, end_sx, end_sy, fill="yellow", arrow=tk.LAST, width=2)

        self.needs_redraw = False

    def reset_map_data(self):
        self.bot_world_x = 0.0
        self.bot_world_y = 0.0
        self.bot_heading = 90.0
        self.path_points = [(0.0, 0.0)]
        self.scan_points = []
        self.cliff_points = []
        self.boundary_points = []
        self.tracking_object = False
        self.view_cx = 0.0
        self.view_cy = 20.0 # Offset camera slightly up so bot starts near bottom
        self.queue_redraw()

    # --- Communication & Data Processing ---
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
        try:
            # Process all pending messages
            while not self.msg_queue.empty():
                msg = self.msg_queue.get()
                self.parse_message(msg)
                
            # Only execute the heavy drawing function once per frame, if needed
            if self.needs_redraw:
                self.perform_redraw()
        except Exception as e:
            # If a rendering or parsing error happens, catch it so the GUI loop doesn't permanently die
            self.log_event(f"[GUI ERROR] Caught exception in main loop: {e}")
            self.needs_redraw = False 
            
        self.root.after(50, self.process_queue)

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

    def parse_message(self, msg):
        if not msg: return
        lower_msg = msg.lower()

        scan_match = re.search(r"Angle:\s*(\d+).*?Distance:\s*([0-9.]+).*?Raw IR Value:\s*(\d+)", msg, re.IGNORECASE)
        if scan_match:
            self.scan_label.config(text=msg)
            angle = float(scan_match.group(1))
            ping_distance = float(scan_match.group(2))
            ir_value = int(scan_match.group(3))
            ir_distance = (4727.5 * pow(ir_value, -0.829) / 10)

            # Edge case: If starting a new sweep at angle 0, prime the previous value and skip logic
            if angle <= 2: # Give it a degree or two of leeway
                self.prev_ir_value = ir_value
                self.tracking_object = False
                return
            
            threshold = self.ir_threshold_slider.get()
            ir_delta = ir_value - self.prev_ir_value

            # Detect rising edge (Significant increase = start of object)
            if not self.tracking_object and ir_delta > threshold:
                self.tracking_object = True
                self.log_event(f"[SCAN] Object Edge Found at {angle}°")

            # Detect falling edge (Significant decrease = end of object)
            elif self.tracking_object and ir_delta < -threshold:
                self.tracking_object = False
                self.log_event(f"[SCAN] Object Ended at {angle}°")

            # If we are currently tracking an object, plot its Ping distance!
            if self.tracking_object:
                self.add_scan_point(angle, ping_distance)

            # Store current value for the next loop
            self.prev_ir_value = ir_value
            return

            # distance += 17
            # ir_distance += 17
            # print(ir_distance)

            # if ((distance - 17) < 80 and ir_distance < 30):
            #     self.draw_scan_point(angle, ir_distance)
            # else:
            #     self.draw_scan_point(angle, distance)
            # return 

        # 2. Parse Hazard Detections (Cliff/Boundary)
        if "cliff" in lower_msg or "boundary" in lower_msg or "object detected" in lower_msg or "bump":
            sensor_angle = 0 # Default (Front Center)
            ping_trigger = False
            # Check specific sensors (check front left/right before generic left/right)
            if "bump" in lower_msg and "left" in lower_msg and "right" in lower_msg:
                sensor_angle = 0
            elif "bump" in lower_msg and "left" in lower_msg:
                sensor_angle = 90
            elif "bump" in lower_msg and "right" in lower_msg:
                sensor_angle = -90
            elif "front left" in lower_msg:
                sensor_angle = 45
            elif "front right" in lower_msg:
                sensor_angle = -45
            elif "left" in lower_msg:
                sensor_angle = 90
            elif "right" in lower_msg:
                sensor_angle = -90
            elif "object detected" in lower_msg:
                self.add_scan_point(90, 10)
                ping_trigger = True
            
            is_bump = "bump" in lower_msg
            is_cliff = "cliff" in lower_msg
            if not ping_trigger:
                self.add_hazard_point(sensor_angle, is_cliff, is_bump)

        move_match = re.search(r"(forward|backward|moved?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if move_match:
            direction = move_match.group(1)
            amount = float(move_match.group(2))
            if "backward" in direction: amount = -amount
            if "mm" in lower_msg: amount /= 10.0

            # Math for updating world position
            rad = math.radians(self.bot_heading)
            self.bot_world_x += amount * math.cos(rad)
            self.bot_world_y += amount * math.sin(rad) 
            self.path_points.append((self.bot_world_x, self.bot_world_y))
            self.queue_redraw()

        turn_match = re.search(r"(left|right|turned?)\s*(-?\d+(?:\.\d+)?)", lower_msg)
        if turn_match:
            direction = turn_match.group(1)
            amount = float(turn_match.group(2))
            if "right" in direction: amount = -amount 

            self.bot_heading += amount
            self.bot_heading %= 360
            self.queue_redraw()

        if any(keyword in lower_msg for keyword in ["bump", "cliff", "move", "turn", "system", "error", "tx", "scan"]):
            self.log_event(msg)
        else:
            self.log_event(f"[RX] {msg}")

    def add_scan_point(self, servo_angle, distance_cm):
        # 1. Find the exact world coordinates of the scanner on the front of the bot
        rad_heading = math.radians(self.bot_heading)
        scanner_x = self.bot_world_x + self.sensor_offset_cm * math.cos(rad_heading)
        scanner_y = self.bot_world_y + self.sensor_offset_cm * math.sin(rad_heading)

        # 2. Plot the object relative to that scanner
        world_angle = self.bot_heading + (servo_angle - 90)
        rad_scan = math.radians(world_angle)
        
        obj_x = scanner_x + distance_cm * math.cos(rad_scan)
        obj_y = scanner_y + distance_cm * math.sin(rad_scan)

        self.scan_points.append((obj_x, obj_y))
        self.queue_redraw()

    def add_hazard_point(self, relative_angle, is_cliff, is_bump):
        """Calculates the world coordinates of the hazard at the edge of the bot"""
        # Calculate world angle by adding bot's heading to the relative sensor angle
        world_angle = self.bot_heading + relative_angle
        rad_hazard = math.radians(world_angle)
        
        # Hazard is at the perimeter of the bot (distance = bot_radius_cm)
        hazard_x = self.bot_world_x + self.bot_radius_cm * math.cos(rad_hazard)
        hazard_y = self.bot_world_y + self.bot_radius_cm * math.sin(rad_hazard)
        
        if is_cliff:
            self.cliff_points.append((hazard_x, hazard_y))
        elif is_bump:
            self.bump_points.append((hazard_x, hazard_y))
        else:
            self.boundary_points.append((hazard_x, hazard_y))
            
        self.queue_redraw()

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
