import re
import tkinter as tk
from tkinter.scrolledtext import ScrolledText
import subprocess
import threading


class RaidGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RAID Visualizer")
        self.root.geometry("1250x920")

        # RAID configuration (VideoDisplayExample)
        self.num_data_disks = 3
        self.num_total_disks = 4          # 3 data + 1 parity
        self.num_stripes = 4
        self.parity_disk_index = 3

        # Track disk states and label widgets
        self.disk_states = {i: "UNKNOWN" for i in range(self.num_total_disks)}
        self.block_labels = {}   # (disk_index, stripe_index) -> Label
        self.disk_header_labels = {}
        self.current_highlights = []

        # ------------------------------
        # RAID layout visualization
        # ------------------------------
        self.layout_frame = tk.LabelFrame(root, text="RAID Layout Visualization", padx=10, pady=10)
        self.layout_frame.pack(fill="x", padx=10, pady=(10, 5))

        self.draw_layout()

        # ------------------------------
        # Interaction mode selection
        # ------------------------------
        self.mode_var = tk.StringVar(value="buttons")

        self.mode_frame = tk.LabelFrame(root, text="Interaction Mode", padx=10, pady=8)
        self.mode_frame.pack(fill="x", padx=10, pady=5)

        tk.Radiobutton(
            self.mode_frame,
            text="Button Controls",
            variable=self.mode_var,
            value="buttons",
            command=self.update_mode
        ).pack(side="left", padx=(0, 12))

        tk.Radiobutton(
            self.mode_frame,
            text="Command Line",
            variable=self.mode_var,
            value="command",
            command=self.update_mode
        ).pack(side="left", padx=(0, 12))

        # ------------------------------
        # Control panel
        # ------------------------------
        self.control_frame = tk.LabelFrame(root, text="Controls", padx=10, pady=10)

        # fixed columns for clean alignment
        for i in range(5):
            self.control_frame.columnconfigure(i, weight=0)

        # make the main entry column a fixed minimum width so all three left entries match exactly
        self.control_frame.columnconfigure(1, minsize=260)

        # Row 0: top buttons
        self.status_button = tk.Button(
            self.control_frame, text="Refresh Status", width=14, command=self.send_status
        )
        self.status_button.grid(row=0, column=0, padx=(8, 8), pady=(6, 12), sticky="w")

        self.exit_button = tk.Button(
            self.control_frame, text="Exit", width=10, command=self.send_exit
        )
        self.exit_button.grid(row=0, column=1, padx=(8, 8), pady=(6, 12), sticky="w")

        # Row 1: kill disk
        tk.Label(self.control_frame, text="Disk ID:", width=14, anchor="e").grid(
            row=1, column=0, padx=(8, 8), pady=6, sticky="e"
        )
        self.kill_entry = tk.Entry(self.control_frame)
        self.kill_entry.grid(row=1, column=1, padx=(0, 16), pady=6, sticky="ew")

        self.kill_button = tk.Button(
            self.control_frame, text="Kill Disk", width=12, command=self.send_kill
        )
        self.kill_button.grid(row=1, column=2, padx=(0, 8), pady=6, sticky="w")

        # Row 2: read block
        tk.Label(self.control_frame, text="Read Block ID:", width=14, anchor="e").grid(
            row=2, column=0, padx=(8, 8), pady=6, sticky="e"
        )
        self.read_entry = tk.Entry(self.control_frame)
        self.read_entry.grid(row=2, column=1, padx=(0, 16), pady=6, sticky="ew")

        self.read_button = tk.Button(
            self.control_frame, text="Read Block", width=12, command=self.send_read
        )
        self.read_button.grid(row=2, column=2, padx=(0, 8), pady=6, sticky="w")

        # Row 3: write block + local file
        tk.Label(self.control_frame, text="Write Block ID:", width=14, anchor="e").grid(
            row=3, column=0, padx=(8, 8), pady=6, sticky="e"
        )

        self.write_block_entry = tk.Entry(self.control_frame)
        self.write_block_entry.grid(row=3, column=1, padx=(0, 6), pady=6, sticky="ew")

        # compact sub-row starting immediately after column 1
        self.write_extra_frame = tk.Frame(self.control_frame)
        self.write_extra_frame.grid(row=3, column=2, columnspan=3, padx=(0, 0), pady=6, sticky="w")

        tk.Label(self.write_extra_frame, text="Local File:", width=10, anchor="e").pack(
            side="left", padx=(0, 2)
        )

        self.write_file_entry = tk.Entry(self.write_extra_frame, width=18)
        self.write_file_entry.pack(side="left", padx=(0, 6))

        self.write_button = tk.Button(
            self.write_extra_frame, text="Write Block", width=12, command=self.send_write
        )
        self.write_button.pack(side="left")

        # ------------------------------
        # Manual command input
        # ------------------------------
        self.input_frame = tk.LabelFrame(root, text="Command Line Input", padx=10, pady=10)

        self.command_entry = tk.Entry(self.input_frame, width=80)
        self.command_entry.pack(side="left", padx=(0, 10), expand=True, fill="x")
        self.command_entry.bind("<Return>", self.send_command)

        self.send_button = tk.Button(self.input_frame, text="Send", width=10, command=self.send_command)
        self.send_button.pack(side="left")

        # ------------------------------
        # Output area
        # ------------------------------
        self.output_frame = tk.LabelFrame(root, text="System Output", padx=5, pady=5)
        self.output_frame.pack(fill="both", expand=True, padx=10, pady=5)

        self.output_box = ScrolledText(self.output_frame, width=120, height=30, state="disabled")
        self.output_box.pack(fill="both", expand=True)

        # Color tags
        self.output_box.tag_config("CMD", foreground="#1f77b4")
        self.output_box.tag_config("FLOW", foreground="#9467bd")
        self.output_box.tag_config("INFO", foreground="#2ca02c")
        self.output_box.tag_config("ERROR", foreground="#d62728")
        self.output_box.tag_config("STATUS", foreground="#ff7f0e")
        self.output_box.tag_config("INPUT", foreground="#7f7f7f")
        self.output_box.tag_config("BANNER", foreground="#444444")
        self.output_box.tag_config("DATA", foreground="#000000")

        # ------------------------------
        # Start backend process
        # ------------------------------
        self.process = subprocess.Popen(
            ["./raid_sim"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        # Welcome message
        self.append_output("====================================================\n", "BANNER")
        self.append_output(" RAID Visualizer started\n", "BANNER")
        self.append_output(" Backend process launched successfully\n", "BANNER")
        self.append_output("====================================================\n", "BANNER")
        self.append_output("[INFO] Available commands: status, kill <disk>, rb <block>, wb <block> <file>, exit\n")
        self.append_output("[INFO] The top panel visualizes RAID-4 block layout and parity structure.\n")
        self.append_output("[INFO] Disk headers will turn green/red based on alive/dead status.\n")
        self.append_output("[INFO] Read/write commands will highlight the corresponding data block and parity block.\n\n")

        # Start in button mode
        self.update_mode()

        # Start reader thread
        self.reader_thread = threading.Thread(target=self.read_output, daemon=True)
        self.reader_thread.start()

    # -------------------------------------------------
    # RAID layout drawing
    # -------------------------------------------------
    def draw_layout(self):
        for widget in self.layout_frame.winfo_children():
            widget.destroy()

        self.block_labels.clear()
        self.disk_header_labels.clear()

        title_frame = tk.Frame(self.layout_frame)
        title_frame.pack(fill="x", pady=(0, 4))

        tk.Label(
            title_frame,
            text="Logical blocks are striped across Disk 0, Disk 1, and Disk 2.\nParity for each stripe is stored on Disk 3.",
            font=("Arial", 11)
        ).pack()

        table_frame = tk.Frame(self.layout_frame)
        table_frame.pack(pady=5)

        tk.Label(table_frame, text="", width=10).grid(row=0, column=0, padx=5, pady=4)

        disk_names = ["Disk 0", "Disk 1", "Disk 2", "Parity Disk\n(Disk 3)"]

        for d in range(self.num_total_disks):
            lbl = tk.Label(
                table_frame,
                text=disk_names[d],
                width=16,
                height=2,
                font=("Arial", 12, "bold"),
                relief="solid",
                bd=1,
                bg="#dddddd"
            )
            lbl.grid(row=0, column=d + 1, padx=8, pady=4)
            self.disk_header_labels[d] = lbl

        logical_block = 0
        for stripe in range(self.num_stripes):
            tk.Label(
                table_frame,
                text=f"stripe {stripe}",
                width=10,
                font=("Arial", 11)
            ).grid(row=stripe + 1, column=0, padx=5, pady=4)

            for d in range(self.num_data_disks):
                block_label = tk.Label(
                    table_frame,
                    text=str(logical_block),
                    width=16,
                    height=2,
                    relief="solid",
                    bd=1,
                    font=("Arial", 12),
                    bg="white"
                )
                block_label.grid(row=stripe + 1, column=d + 1, padx=8, pady=2)
                self.block_labels[(d, stripe)] = block_label
                logical_block += 1

            parity_label = tk.Label(
                table_frame,
                text=f"P{stripe}",
                width=16,
                height=2,
                relief="solid",
                bd=1,
                font=("Arial", 12, "bold"),
                bg="#f3f3f3"
            )
            parity_label.grid(row=stripe + 1, column=self.parity_disk_index + 1, padx=8, pady=2)
            self.block_labels[(self.parity_disk_index, stripe)] = parity_label

        legend_frame = tk.Frame(self.layout_frame)
        legend_frame.pack(fill="x", pady=(8, 0))

        tk.Label(legend_frame, text="Legend:", font=("Arial", 10, "bold")).pack(side="left", padx=(0, 10))
        tk.Label(legend_frame, text="Alive", bg="#c8f7c5", width=10, relief="solid").pack(side="left", padx=5)
        tk.Label(legend_frame, text="Dead", bg="#f7c5c5", width=10, relief="solid").pack(side="left", padx=5)
        tk.Label(legend_frame, text="Highlighted access", bg="#fff59d", width=16, relief="solid").pack(side="left", padx=5)

    # -------------------------------------------------
    # Interaction mode switching
    # -------------------------------------------------
    def update_mode(self):
        self.control_frame.pack_forget()
        self.input_frame.pack_forget()

        if self.mode_var.get() == "buttons":
            self.control_frame.pack(fill="x", padx=10, pady=5, before=self.output_frame)
        else:
            self.input_frame.pack(fill="x", padx=10, pady=(5, 10), before=self.output_frame)

    # -------------------------------------------------
    # Highlight logic
    # -------------------------------------------------
    def clear_highlights(self):
        for (disk_index, stripe_index) in self.current_highlights:
            label = self.block_labels.get((disk_index, stripe_index))
            if label:
                if disk_index == self.parity_disk_index:
                    label.config(bg="#f3f3f3")
                else:
                    label.config(bg="white")
        self.current_highlights.clear()

    def highlight_logical_block(self, logical_block):
        self.clear_highlights()

        if logical_block < 0:
            return

        data_disk = logical_block % self.num_data_disks
        stripe = logical_block // self.num_data_disks

        if stripe >= self.num_stripes:
            return

        data_label = self.block_labels.get((data_disk, stripe))
        parity_label = self.block_labels.get((self.parity_disk_index, stripe))

        if data_label:
            data_label.config(bg="#fff59d")
            self.current_highlights.append((data_disk, stripe))

        if parity_label:
            parity_label.config(bg="#fff59d")
            self.current_highlights.append((self.parity_disk_index, stripe))

    # -------------------------------------------------
    # Disk status updates
    # -------------------------------------------------
    def update_disk_status(self, disk_num, state):
        self.disk_states[disk_num] = state
        label = self.disk_header_labels.get(disk_num)
        if not label:
            return

        if state == "ALIVE":
            label.config(bg="#c8f7c5")
        elif state == "DEAD":
            label.config(bg="#f7c5c5")
        else:
            label.config(bg="#dddddd")

    # -------------------------------------------------
    # Output handling
    # -------------------------------------------------
    def append_output(self, text, forced_tag=None):
        self.output_box.config(state="normal")

        tag = forced_tag
        stripped = text.lstrip()

        if tag is None:
            if stripped.startswith("[CMD]"):
                tag = "CMD"
            elif stripped.startswith("[FLOW]"):
                tag = "FLOW"
            elif stripped.startswith("[INFO]"):
                tag = "INFO"
            elif stripped.startswith("[ERROR]"):
                tag = "ERROR"
            elif stripped.startswith("[STATUS]"):
                tag = "STATUS"
            elif stripped.startswith("raid>"):
                tag = "INPUT"

        if tag:
            self.output_box.insert(tk.END, text, tag)
        else:
            self.output_box.insert(tk.END, text)

        self.output_box.see(tk.END)
        self.output_box.config(state="disabled")

        self.parse_line_for_visual_updates(text)

    def parse_line_for_visual_updates(self, line):
        status_match = re.search(r"\[STATUS\]\s+disk=(\d+)\s+state=([A-Z]+)", line)
        if status_match:
            disk_num = int(status_match.group(1))
            state = status_match.group(2)
            self.update_disk_status(disk_num, state)
            return

        cmd_read_match = re.search(r"\[CMD\]\s+rb\s+(\d+)", line)
        if cmd_read_match:
            logical_block = int(cmd_read_match.group(1))
            self.highlight_logical_block(logical_block)
            return

        cmd_write_match = re.search(r"\[CMD\]\s+wb\s+(\d+)", line)
        if cmd_write_match:
            logical_block = int(cmd_write_match.group(1))
            self.highlight_logical_block(logical_block)
            return

    def read_output(self):
        for line in self.process.stdout:
            self.root.after(0, self.append_output, line)

    # -------------------------------------------------
    # Command sending
    # -------------------------------------------------
    def send_text_command(self, cmd):
        if not cmd.strip():
            return

        self.append_output(f"raid> {cmd}\n", "INPUT")

        if self.process.stdin:
            self.process.stdin.write(cmd + "\n")
            self.process.stdin.flush()

    def send_command(self, event=None):
        cmd = self.command_entry.get().strip()
        if not cmd:
            return

        self.send_text_command(cmd)
        self.command_entry.delete(0, tk.END)

    def send_status(self):
        self.send_text_command("status")

    def send_kill(self):
        disk_id = self.kill_entry.get().strip()
        if not disk_id:
            self.append_output("[ERROR] Please enter a disk ID before clicking Kill Disk.\n")
            return
        self.send_text_command(f"kill {disk_id}")

    def send_read(self):
        block_id = self.read_entry.get().strip()
        if not block_id:
            self.append_output("[ERROR] Please enter a block ID before clicking Read Block.\n")
            return
        self.send_text_command(f"rb {block_id}")

    def send_write(self):
        block_id = self.write_block_entry.get().strip()
        filename = self.write_file_entry.get().strip()

        if not block_id or not filename:
            self.append_output("[ERROR] Please enter both block ID and local file name before clicking Write Block.\n")
            return

        self.send_text_command(f"wb {block_id} {filename}")

    def send_exit(self):
        self.send_text_command("exit")
        self.root.after(200, self.root.destroy)

    # -------------------------------------------------
    # Close
    # -------------------------------------------------
    def on_close(self):
        try:
            if self.process.stdin:
                self.process.stdin.write("exit\n")
                self.process.stdin.flush()
        except Exception:
            pass

        try:
            self.process.terminate()
        except Exception:
            pass

        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = RaidGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()
