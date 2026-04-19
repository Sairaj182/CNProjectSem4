import tkinter as tk
from tkinter import filedialog, scrolledtext, ttk
import subprocess
import threading
import os
import shutil

# -------------------------
# GLOBAL VARIABLES
# -------------------------
server_process = None
packet_count = 0
corrupt_count = 0

# -------------------------
# WSL PATH HELPER
# -------------------------
def get_wsl_path():
    win_path = os.getcwd()
    drive = win_path[0].lower()
    path = win_path[2:].replace("\\", "/")
    return f"/mnt/{drive}{path}"

# -------------------------
# SAFE UI UPDATE
# -------------------------
def safe_ui(func):
    root.after(0, func)

# -------------------------
# LOG FUNCTION
# -------------------------
def log(message, tag="info"):
    log_box.insert(tk.END, message, tag)
    log_box.see(tk.END)

# -------------------------
# START SERVER
# -------------------------
def start_server():
    global server_process

    if server_process is not None:
        log("⚠ Server already running\n", "warn")
        return

    # Check binary exists before launching
    if not os.path.exists("server"):
        log("⚠ './server' binary not found — run: gcc server.c -o server\n", "warn")
        return

    def run_server():
        global server_process, corrupt_count

        wsl_dir = get_wsl_path()

        server_process = subprocess.Popen(
            ["wsl", "bash", "-c", f"cd {wsl_dir} && ./server"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        for line in server_process.stdout:
            line = line.strip()

            if "CORRUPTED" in line or "corrupted" in line:
                corrupt_count += 1
                safe_ui(lambda c=corrupt_count: corrupt_label.config(
                    text=f"Corrupted Packets: {c}"))
                safe_ui(lambda l=line: log("❌ " + l + "\n", "error"))

            elif "END sentinel" in line:
                safe_ui(lambda l=line: log("🏁 " + l + "\n", "warn"))

            elif "OK" in line or "Received packet" in line:
                safe_ui(lambda l=line: log("📡 " + l + "\n", "success"))

            else:
                safe_ui(lambda l=line: log("📡 " + l + "\n", "info"))

        for err in server_process.stderr:
            safe_ui(lambda e=err: log("ERROR: " + e + "\n", "error"))

        # Reset so user can restart server without relaunching app
        server_process = None
        safe_ui(lambda: log("🔴 Server stopped\n", "warn"))

    threading.Thread(target=run_server, daemon=True).start()
    log("✅ Server started\n", "success")

# -------------------------
# SEND FILE
# -------------------------
def send_file():
    global packet_count, corrupt_count

    if not selected_file.get():
        log("⚠ Please select a file first\n", "warn")
        return

    # Check binary exists before launching
    if not os.path.exists("client"):
        log("⚠ './client' binary not found — run: gcc client.c -o client\n", "warn")
        return

    src = selected_file.get()
    dst = "internalInput.txt"

    if os.path.abspath(src) != os.path.abspath(dst):
        shutil.copy(src, dst)
    else:
        log("⚠ File already selected as internalInput.txt\n", "warn")

    file_size = os.path.getsize("internalInput.txt")
    size_label.config(text=f"File Size: {file_size} bytes")

    # Reset counters
    packet_count = 0
    corrupt_count = 0
    packet_label.config(text="Packets Sent: 0")
    corrupt_label.config(text="Corrupted Packets: 0")
    progress['value'] = 0

    def run_client():
        global packet_count

        wsl_dir = get_wsl_path()

        process = subprocess.Popen(
            ["wsl", "bash", "-c", f"cd {wsl_dir} && ./client"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        for line in process.stdout:
            line = line.strip()

            if "Sent packet" in line:
                packet_count += 1
                safe_ui(lambda p=packet_count: packet_label.config(
                    text=f"Packets Sent: {p}"))
                safe_ui(lambda: progress.step(2))
                safe_ui(lambda l=line: log("🚀 " + l + "\n", "success"))

            elif "END sentinel" in line:
                safe_ui(lambda l=line: log("🏁 " + l + "\n", "warn"))

            else:
                safe_ui(lambda l=line: log("🚀 " + l + "\n", "info"))

        for err in process.stderr:
            safe_ui(lambda e=err: log("ERROR: " + e + "\n", "error"))

        safe_ui(lambda: progress.config(value=100))
        safe_ui(lambda: log("🎉 File transfer complete\n", "success"))

    threading.Thread(target=run_client, daemon=True).start()

# -------------------------
# FILE BROWSER
# -------------------------
def browse_file():
    file_path = filedialog.askopenfilename()
    if file_path:
        selected_file.set(file_path)
        log(f"📂 Selected file: {file_path}\n", "info")

# -------------------------
# UI SETUP
# -------------------------
root = tk.Tk()
root.title("UDP File Transfer Dashboard")
root.geometry("800x580")
root.configure(bg="#1e1e2f")

tk.Label(root, text="📡 UDP File Transfer System",
         font=("Arial", 18, "bold"),
         bg="#1e1e2f", fg="white").pack(pady=10)

selected_file = tk.StringVar()

frame = tk.Frame(root, bg="#1e1e2f")
frame.pack(pady=10)

tk.Entry(frame, textvariable=selected_file, width=55).pack(side=tk.LEFT, padx=5)

tk.Button(frame, text="Browse", command=browse_file,
          bg="#4CAF50", fg="white", width=10).pack(side=tk.LEFT)

btn_frame = tk.Frame(root, bg="#1e1e2f")
btn_frame.pack(pady=10)

tk.Button(btn_frame, text="Start Server",
          command=start_server,
          bg="#2196F3", fg="white", width=15).pack(side=tk.LEFT, padx=10)

tk.Button(btn_frame, text="Send File",
          command=send_file,
          bg="#FF9800", fg="white", width=15).pack(side=tk.LEFT, padx=10)

info_frame = tk.Frame(root, bg="#1e1e2f")
info_frame.pack(pady=5)

size_label = tk.Label(info_frame, text="File Size: 0 bytes",
                      bg="#1e1e2f", fg="white")
size_label.pack()

packet_label = tk.Label(info_frame, text="Packets Sent: 0",
                        bg="#1e1e2f", fg="white")
packet_label.pack()

corrupt_label = tk.Label(info_frame, text="Corrupted Packets: 0",
                         bg="#1e1e2f", fg="red")
corrupt_label.pack()

progress = ttk.Progressbar(root, orient="horizontal",
                           length=500, mode="determinate")
progress.pack(pady=10)

log_box = scrolledtext.ScrolledText(root, width=95, height=18,
                                    bg="black", fg="white",
                                    font=("Consolas", 10))
log_box.pack(pady=10)

log_box.tag_config("info", foreground="white")
log_box.tag_config("success", foreground="lime")
log_box.tag_config("error", foreground="red")
log_box.tag_config("warn", foreground="yellow")

log("🚀 System Ready\n", "success")

root.mainloop()