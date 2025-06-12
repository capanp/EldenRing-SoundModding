import os
import csv
import tkinter as tk
from tkinter import filedialog, messagebox

WEM_MAP_PATH = './wemmap/wem_talk_map_v.0.0.2.csv'

class WemFinder:
    def __init__(self, root):
        self.root = root
        self.elden_ring_path = None
        self.create_widgets()

    def create_widgets(self):
        self.frame = tk.Frame(self.root, bg="#222")
        self.frame.pack(fill="both", expand=True)

        self.label = tk.Label(self.frame, text="Select Elden Ring path", bg="#222", fg="white")
        self.label.pack(pady=10)

        self.select_btn = tk.Button(self.frame, text="Browse", command=self.select_path)
        self.select_btn.pack()

        self.id_entry_label = tk.Label(self.frame, text="Enter ID", bg="#222", fg="white")
        self.id_entry = tk.Entry(self.frame)

        self.find_btn = tk.Button(self.frame, text="Find", command=self.find_wem)

    def select_path(self):
        selected_dir = filedialog.askdirectory(title="Select Elden Ring folder")
        if not selected_dir:
            return

        wem_path = os.path.join(selected_dir, "sd", "enus", "wem")
        if not os.path.exists(wem_path):
            messagebox.showerror("Error", "Please unpack Elden Ring sound files via UMX Packer")
            return

        self.elden_ring_path = selected_dir
        self.label.config(text=f"Selected: {selected_dir}")

        self.id_entry_label.pack(pady=(20, 5))
        self.id_entry.pack()
        self.find_btn.pack(pady=10)

    def find_wem(self):
        input_id = self.id_entry.get().strip()
        if not input_id.isdigit():
            messagebox.showerror("Error", "Please enter a numeric ID.")
            return

        wem_id = self.lookup_wem_id(input_id)
        if wem_id is None:
            messagebox.showerror("Error", "ID not found in CSV.")
            return

        folder_prefix = str(wem_id)[:2]
        wem_file_path = os.path.join(
            self.elden_ring_path, "sd", "enus", "wem", folder_prefix, f"{wem_id}.wem"
        )

        if os.path.exists(wem_file_path):
            os.startfile(os.path.normpath(wem_file_path))  # Opens folder with file selected
        else:
            messagebox.showerror("Error", "WEM file not found.")

    def lookup_wem_id(self, target_id):
        try:
            with open(WEM_MAP_PATH, newline='', encoding='utf-8') as csvfile:
                reader = csv.reader(csvfile)
                for row in reader:
                    if row[0] == target_id:
                        return row[2]
        except Exception as e:
            messagebox.showerror("Error", f"Failed to read CSV: {e}")
        return None


if __name__ == "__main__":
    root = tk.Tk()
    root.title("Elden Ring WEM Finder")
    root.geometry("500x300")
    app = WemFinder(root)
    root.mainloop()
