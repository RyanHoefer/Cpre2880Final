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
        self.root.geometry("600x750")
        
        # Thread-safe queue for incoming socket messages
        self.msg_queue = queue.Queue()
        self.running = True
        self.sock = None

        self.setup_ui()
        
        # Start the GUI update loop
        self.root.after(100, self.process_queue)

    def setup_ui(self):
        # --- Connection Frame ---
        conn_frame = tk.Frame(self.root)
        conn_frame.pack(pady=10)
        
        tk.Label(conn_frame, text="Cybot IP:").grid(row=0, column=0, padx=5)
        self.ip_entry = tk.Entry(conn_frame, width=15)
        self.ip_entry.insert(0, "192.168.1.1") # Default IP, change as needed
        self.ip_entry.grid(row=0, column=1, padx=5)
        
        self.connect_btn = tk.Button(conn_frame, text="Connect", command=self.connect_to_bot)
        self.connect_btn.grid(row=0, column=2, padx=5)

        # --- Real-Time Data Label ---
        self.scan_label = tk.Label(self.root, text="Angle: ---  Distance: ---  Raw IR Value: ---", font=("Consolas", 14, "bold"))
        self.scan_label.pack(pady=10)

        # --- Field View (Canvas) ---
        tk.Label(self.root, text="Field View (Sensor Sweep)").pack()
        self.canvas_width = 500
        self.canvas_height = 300
        self.canvas = tk.Canvas(self.root, width=self.canvas_width, height=self.canvas_height, bg="black")
        self.canvas.pack()
        
        btn_frame = tk.Frame(self.root)
        btn_frame.pack(pady=5)
        tk.Button(btn_frame, text="Clear Field", command=self.clear_canvas).pack()
        
        self.clear_canvas() # Draw initial bot position

        # --- Event Log ---
        tk.Label(self.root, text="Event Log (Bumps, Cliffs, Movement)").pack()
        self.log_text = tk.Text(self.root, height=12, width=70, state=tk.DISABLED, bg="#f0f0f0")
        self.log_text.pack(pady=5)

    def clear_canvas(self):
        self.canvas.delete("all")
        # Draw the Cybot at the bottom center
        cx = self.canvas_width / 2
        cy = self.canvas_height - 10
        self.canvas.create_oval(cx - 15, cy - 15, cx + 15, cy + 15, fill="blue", outline="white")
        self.canvas.create_text(cx, cy, text="BOT", fill="white", font=("Arial", 8))

    def connect_to_bot(self):
        ip = self.ip_entry.get()
        port = 288
        
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(3.0) # 3 second timeout for connection attempt
            self.sock.connect((ip, port))
            self.sock.settimeout(None) # Remove timeout for continuous listening
            
            self.log_event(f"[SYSTEM] Successfully connected to {ip}:{port}")
            self.connect_btn.config(state=tk.DISABLED)
            
            # Start background thread to listen for data
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
                # Process line by line
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    self.msg_queue.put(line.strip())
            except Exception as e:
                self.msg_queue.put(f"[SYSTEM] Disconnected: {e}")
                break
                
        self.connect_btn.config(state=tk.NORMAL)

    def process_queue(self):
        # Process all pending messages in the queue to update the GUI
        while not self.msg_queue.empty():
            msg = self.msg_queue.get()
            self.parse_message(msg)
            
        # Reschedule the queue check
        self.root.after(50, self.process_queue)

    def parse_message(self, msg):
        if not msg:
            return

        # Check for Scan Data
        # Regex expects format similar to: "Angle: 90  Distance: 45.2  Raw IR Value: 1200"
        scan_match = re.search(r"Angle:\s*(\d+).*?Distance:\s*([0-9.]+).*?Raw IR Value:\s*(\d+)", msg, re.IGNORECASE)
        
        if scan_match:
            # Update the Real-Time Label
            self.scan_label.config(text=msg)
            
            # Extract values for drawing
            angle = float(scan_match.group(1))
            distance = float(scan_match.group(2))
            self.draw_scan_point(angle, distance)
            return

        # Check for Events (Bump, Cliff, Movement)
        lower_msg = msg.lower()
        if any(keyword in lower_msg for keyword in ["bump", "cliff", "move", "system", "error"]):
            self.log_event(msg)
        else:
            # Catch-all for unrecognized formatted text
            self.log_event(f"[RAW] {msg}")

    def draw_scan_point(self, angle, distance):
        # Assuming angle is 0 to 180 degrees, where 90 is straight ahead
        # Center bottom of canvas is (250, 290)
        cx = self.canvas_width / 2
        cy = self.canvas_height - 10
        
        # Scaling factor: Adjust this depending on how you want cm/inches to map to pixels
        scale = 3.0 
        pixel_dist = distance * scale

        # Convert polar to Cartesian coordinates
        # Math note: 0 degrees is right, 180 is left. Y is inverted in Tkinter (0 is top).
        rad = math.radians(angle)
        x = cx + (pixel_dist * math.cos(rad))
        y = cy - (pixel_dist * math.sin(rad))

        # Only draw if it's within the canvas bounds
        if 0 <= x <= self.canvas_width and 0 <= y <= self.canvas_height:
            # Draw a small red dot representing the object
            self.canvas.create_oval(x-2, y-2, x+2, y+2, fill="red", outline="red")

    def log_event(self, text):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, text + "\n")
        self.log_text.see(tk.END) # Auto-scroll to bottom
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