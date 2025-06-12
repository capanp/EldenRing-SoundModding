import tkinter as tk
from tkinter import messagebox, filedialog
import webbrowser
import os
import shutil
import csv
import subprocess

WEM_MAP_PATH = './wemmap/wem_talk_map_v.0.0.2.csv'

BG_COLOR = "#191919"
BUTTON_COLOR = "#292929"
PANEL_COLOR = "#2a2a2a"
TEXT_COLOR = "white"
SELECTED_COLOR = "#424242"
BORDER_COLOR = "#292929"
SIDE_COLOR = "#161616"

ELDEN_RING_PATH = ""

WIDTH = 540
HEIGHT = 870

class CustomApp(tk.Tk):
    def __init__(self):
        super().__init__()

        self.overrideredirect(True)
        self.resizable(False, False)
        self.configure(bg=BG_COLOR)

        self.center_window(WIDTH, HEIGHT)

        self.offset_x = 0
        self.offset_y = 0

        self.create_title_bar()
        self.create_sidebar()
        self.create_main_area()

        self.category_functions = {
            "Welcome": self.create_first_content,
            "Find Dialogue Wem": self.create_second_content,
            "Find Unknown Wem": self.create_third_content,
            "Convert Files": self.create_fourth_content,
        }

        # Başlangıçta ilk kategori gösterilsin
        self.show_content("Welcome")

    def center_window(self, width, height):
        screen_width = self.winfo_screenwidth()
        screen_height = self.winfo_screenheight()
        x = (screen_width // 2) - (width // 2)
        y = (screen_height // 2) - (height // 2)
        self.geometry(f"{width}x{height}+{x}+{y}")

    def create_title_bar(self):
        self.title_bar = tk.Frame(self, bg=BG_COLOR, height=30)
        self.title_bar.pack(fill="x")

        self.app_name = tk.Label(
            self.title_bar, text="Elden Ring Sound Modding Helper", bg=BG_COLOR,
            fg=TEXT_COLOR, font=("Arial", 10)
        )
        self.app_name.pack(side="left", padx=10)

        self.close_button = tk.Button(
            self.title_bar, text="✕", command=self.destroy,
            bg=BG_COLOR, fg=TEXT_COLOR, bd=0,
            font=("Arial", 12), activebackground=BG_COLOR,
            activeforeground=TEXT_COLOR
        )
        self.close_button.pack(side="right", padx=5, pady=2)

        self.minimize_button = tk.Button(
            self.title_bar, text="—", command=self.iconify,
            bg=BG_COLOR, fg=TEXT_COLOR, bd=0,
            font=("Arial", 14), activebackground=BG_COLOR,
            activeforeground=TEXT_COLOR
        )
        self.minimize_button.pack(side="right", padx=5, pady=2)

        self.title_bar.bind("<Button-1>", self.start_move)
        self.title_bar.bind("<B1-Motion>", self.do_move)

    def start_move(self, event):
        self.offset_x = event.x
        self.offset_y = event.y

    def do_move(self, event):
        x = event.x_root - self.offset_x
        y = event.y_root - self.offset_y
        self.geometry(f"+{x}+{y}")

    def create_sidebar(self):
        self.sidebar = tk.Frame(self, bg=BG_COLOR, width=100)
        self.sidebar.pack(side="left", fill="y")
        
        border_right = tk.Frame(self, bg=PANEL_COLOR, width=2)
        border_right.pack(side="left", fill="y")

        self.buttons = {}
        for katagori in ["Welcome", "Find Dialogue Wem", "Find Unknown Wem", "Convert Files"]:
            button = tk.Button(
                self.sidebar, text=katagori,
                fg=TEXT_COLOR, bg=SIDE_COLOR,
                activebackground=SELECTED_COLOR, relief="flat", bd=0, height=3, width=20,
                command=lambda k=katagori: self.show_content(k)
            )
            button.pack(fill="x")
            border_bottom = tk.Frame(self.sidebar, bg=BORDER_COLOR, height=1)
            border_bottom.pack(fill="both")
            self.buttons[katagori] = button

    def create_main_area(self):
        self.main_area = tk.Frame(self, bg=BG_COLOR)
        self.main_area.pack(side="left", fill="both", expand=True)

        self.title_label = tk.Label(
            self.main_area, text="", fg=TEXT_COLOR, bg=PANEL_COLOR, height=2,
            font=("Helvetica", 16)
        )
        self.title_label.pack(fill="x")

        self.content_frame = tk.Frame(self.main_area, bg=BG_COLOR)
        self.content_frame.pack(fill="both", expand=True)

    def clear_content(self):
        for widget in self.content_frame.winfo_children():
            widget.destroy()

    def show_content(self, katagori):
        self.title_label.config(text=f"{katagori}")
        self.clear_content()

        if katagori in self.category_functions:
            self.category_functions[katagori]()
    def create_first_content(self):
        def open_link():
            webbrowser.open("https://github.com/capanp/EldenRing-SoundModding")  # İlgili GitHub repo linki
        
        label = tk.Label(
            self.content_frame, text="Welcome to the Elden Ring sound modding\nhelper, visit the github page for detailed user guide.\n\nhttps://github.com/capanp/EldenRing-SoundModding", fg=TEXT_COLOR,
            bg=BG_COLOR, font=("Helvetica", 12)
        )
        label.pack(pady=20)
        
        button = tk.Button(
            self.content_frame, text="Visit Github",
            fg=TEXT_COLOR, bg=PANEL_COLOR,
            activebackground=SELECTED_COLOR, relief="flat", bd=0,
            command=open_link
        )
        button.pack(fill="x", pady=5)
        
        label = tk.Label(
            self.content_frame, text="halo, senyorita!", fg=TEXT_COLOR,
            bg=BG_COLOR, font=("Helvetica", 12)
        )
        label.pack(pady=20)

    def create_second_content(self):
        
        frame = tk.Frame(self.content_frame, bg=BG_COLOR)
        frame.pack(fill="both")

        label = tk.Label(frame, text="Select Unpacked Elden Ring Path", fg="white", bg=BG_COLOR,font=("Helvetica", 12))
        label.pack(pady=10)

        select_btn = tk.Button(frame, text="Browse", command=lambda: self.select_path(False), activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR, bd=0, relief="flat", width=20)
        select_btn.pack(pady=(0, 25))

        # Bu widget'ları instance değişkenleri olarak tanımla
        self.id_entry_label = tk.Label(frame, text="Enter a TalkID", fg="white", bg=BG_COLOR,font=("Helvetica", 12))
        self.id_entry = tk.Entry(frame)
        
        self.find_frame = tk.Frame(frame, bg=BG_COLOR)
        
        tk.Button(self.find_frame, text="Find", command=self.find_wem,
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=10).pack(side="left", padx=(0, 5))

        tk.Button(self.find_frame, text="Open Wem Map .csv File", command=lambda: os.startfile(os.path.abspath(WEM_MAP_PATH)),
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=20).pack(side="left", padx=(0, 5))

        self.border_bottom = tk.Frame(frame, bg=BORDER_COLOR, height=1)
        
        # Path seçildikten sonra gösterilecek label
        self.label = tk.Label(frame, text="", bg="#222", fg="white")
        
        border_bottom = tk.Frame(frame, bg=BORDER_COLOR, height=1)
        border_bottom.pack(fill="both")
        
    def select_path(self, id):
        selected_dir = filedialog.askdirectory(title="Select Elden Ring folder")
        if not selected_dir:
            return

        wem_path = os.path.join(selected_dir, "sd", "enus", "wem")
        if not os.path.exists(wem_path):
            messagebox.showerror("Error", "Please unpack Elden Ring sound files via UMX Packer")
            return
        
        def unpack_banks(game_dir):
            source_banks_dir = os.path.join(game_dir, "sd", "enus")
            destination_dir = os.path.join(".", "tools", "banks")

            os.makedirs(destination_dir, exist_ok=True)

            # Tüm .bnk dosyalarını kopyala
            for filename in os.listdir(source_banks_dir):
                if filename.endswith(".bnk"):
                    src_path = os.path.join(source_banks_dir, filename)
                    dest_path = os.path.join(destination_dir, filename)
                    shutil.copy2(src_path, dest_path)
                    print(f"Copied: {src_path} -> {dest_path}")
                    
            for filename in os.listdir(destination_dir):
                if filename.endswith(".bnk"):
                    src_path = os.path.join(destination_dir, filename)
                    subprocess.run('"./tools/bnk2json.exe" "' + src_path + '"', shell=True) 
                    print(f"Unpacked: {src_path}")

            messagebox.showinfo("Success", f".bnk files unpacked to {destination_dir}")

        if(id):
            unpack_banks(selected_dir)

        self.elden_ring_path = selected_dir
        self.label.config(text=f"Selected: {selected_dir}")

        self.id_entry_label.pack(pady=(20, 5))
        self.id_entry.pack()
        self.find_frame.pack(pady=20)
        self.border_bottom.pack(fill="both")

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
    
    def find_unk_wem(id):
        print(id)

    def create_third_content(self):
        label = tk.Label(
            self.content_frame, text="Before you start, make sure that the voice file is not\nthe videodialogue that plays at the beginning(intro)\nor end(outro) of the game. Those voiceovers are embedded\nin the bk2 files, you can change them with rad tools.", fg=TEXT_COLOR,
            bg=BG_COLOR, font=("Helvetica", 12)
        )
        label.pack(pady=20)
            
        border_bottom = tk.Frame(self.content_frame, bg=BORDER_COLOR, height=1)
        border_bottom.pack(fill="x")
        
        label = tk.Label(
            self.content_frame, text="If the voice file you are looking for belongs to\nMalenia, it is in the vcmain.bnk file.", fg=TEXT_COLOR,
            bg=BG_COLOR, font=("Helvetica", 12)
        )
        label.pack(pady=20)
        
        border_bottom = tk.Frame(self.content_frame, bg=BORDER_COLOR, height=1)
        border_bottom.pack(fill="x")
            
        frame = tk.Frame(self.content_frame, bg=BG_COLOR)
        frame.pack(fill="both")
            
        def first_open():
            label = tk.Label(frame, text="Select Unpacked Elden Ring Path For Unpack banks\n(This process will take a few minutes,\nDO NOT close the application,\nif you have any problems clear this path ./tools/banks)", fg="white", bg=BG_COLOR,font=("Helvetica", 12))
            label.pack(pady=10)

            select_btn = tk.Button(frame, text="Browse", command=lambda: self.select_path(True), activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR, bd=0, relief="flat", width=20)
            select_btn.pack(pady=(0, 25))

            # Bu widget'ları instance değişkenleri olarak tanımla
            self.id_entry_label = tk.Label(frame, text="Enter a TalkID", fg="white", bg=BG_COLOR,font=("Helvetica", 12))
            self.id_entry = tk.Entry(frame)
            
            self.find_frame = tk.Frame(frame, bg=BG_COLOR)
            
            tk.Button(self.find_frame, text="Find", command=self.find_unk_wem(),
                    activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                    bd=0, relief="flat", width=10).pack(side="left", padx=(0, 5))
            
            self.border_bottom = tk.Frame(frame, bg=BORDER_COLOR, height=1)
            
            # Path seçildikten sonra gösterilecek label
            self.label = tk.Label(frame, text="", bg="#222", fg="white")
            
            border_bottom = tk.Frame(frame, bg=BORDER_COLOR, height=1)
            border_bottom.pack(fill="both")
            
        def another_open():
            # Bu widget'ları instance değişkenleri olarak tanımla
            self.id_entry_label = tk.Label(frame, text="Enter a TalkID", fg="white", bg=BG_COLOR,font=("Helvetica", 12))
            self.id_entry = tk.Entry(frame)
            self.find_frame = tk.Frame(frame, bg=BG_COLOR)
            
            tk.Button(self.find_frame, text="Find", command=self.find_wem,
                    activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                    bd=0, relief="flat", width=10).pack(side="left", padx=(0, 5))
            
            self.border_bottom = tk.Frame(frame, bg=BORDER_COLOR, height=1)
            
            self.id_entry_label.pack(pady=(20, 5))
            self.id_entry.pack()
            self.find_frame.pack(pady=20)
            self.border_bottom.pack(fill="both")
        
        bnk_files = [file for file in os.listdir("./tools/banks") if file.endswith(".bnk")]
        
        if bnk_files:
            another_open()
        else:
            first_open()
        
    def create_fourth_content(self):
        for widget in self.content_frame.winfo_children():
            widget.destroy()

        import subprocess

        mp3_dir = "./input/mp3"
        name_input_dir = "./input/organize"
        name_output_dir = "./output/organize"
        wav_output_dir = "./output/wav"
        ffmpeg_path = "./tools/ffmpeg.exe"

        os.makedirs(mp3_dir, exist_ok=True)
        os.makedirs(name_input_dir, exist_ok=True)
        os.makedirs(wav_output_dir, exist_ok=True)
        os.makedirs(name_output_dir, exist_ok=True)

        def count_files(path):
            return len([f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))])

        mp3_count = count_files(mp3_dir)
        name_count = count_files(name_input_dir)

        # Başlıklar
        tk.Label(self.content_frame, text=f"{mp3_count} .mp3 files found.", fg="white", bg=BG_COLOR,
                font=("Helvetica", 12)).pack(pady=(10, 5))
        
        def open_folder_if_exists(path):
            abs_path = os.path.abspath(path)
            if os.path.exists(abs_path):
                os.startfile(abs_path)
            else:
                messagebox.showerror("Hata", f"Klasör bulunamadı:\n{abs_path}")

        # MP3 -> WAV dönüştürme
        def convert_mp3_to_wav():
            if not os.path.exists(ffmpeg_path):
                messagebox.showerror("Error", "ffmpeg not found! Place in ./tools/ffmpeg.exe.")
                return

            converted = 0
            for filename in os.listdir(mp3_dir):
                if not filename.lower().endswith(".mp3"):
                    continue
                input_path = os.path.join(mp3_dir, filename)
                output_path = os.path.join(wav_output_dir, os.path.splitext(filename)[0] + ".wav")

                try:
                    subprocess.run([ffmpeg_path, "-y", "-i", input_path, output_path],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    converted += 1
                except Exception as e:
                    print(f"Conversion failed for {filename}: {e}")

            result_label.config(text=f"{converted} mp3 files converted to wav format.")

        convert_frame = tk.Frame(self.content_frame, bg=BG_COLOR)
        convert_frame.pack(pady=(0, 5))
        
        tk.Button(convert_frame, text="Input", command=lambda: open_folder_if_exists(mp3_dir),
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=10).pack(side="left", padx=(0, 5))

        tk.Button(convert_frame, text="Convert to WAV", command=convert_mp3_to_wav,
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=20).pack(side="left", padx=(0, 5))

        tk.Button(convert_frame, text="Output", command=lambda: open_folder_if_exists(wav_output_dir),
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=10).pack(side="left")

        result_label = tk.Label(self.content_frame, text="", fg="lightgreen", bg=BG_COLOR, font=("Helvetica", 10))
        result_label.pack()
        
        border_bottom = tk.Frame(self.content_frame, bg=BORDER_COLOR, height=1)
        border_bottom.pack(fill="x")

        # İsimlendirici Başlığı
        tk.Label(self.content_frame, text=f"{name_count} named files found.", fg="white", bg=BG_COLOR,
                font=("Helvetica", 12)).pack(pady=(20, 5))

        # İsimlendirici Fonksiyonu
        def organize_and_rename_files():
            renamed = 0
            for filename in os.listdir(name_input_dir):
                src_path = os.path.join(name_input_dir, filename)
                if os.path.isdir(src_path):
                    continue

                subdir_name = filename[:2]
                subdir_path = os.path.join(name_output_dir, subdir_name)
                os.makedirs(subdir_path, exist_ok=True)

                new_filename = filename.split('_')[0] + os.path.splitext(filename)[1]
                dest_path = os.path.join(subdir_path, new_filename)

                shutil.copy2(src_path, dest_path)
                renamed += 1

            name_result_label.config(text=f"{renamed} files were organized.")

        organize_frame = tk.Frame(self.content_frame, bg=BG_COLOR)
        organize_frame.pack(pady=(0, 5))
        
        tk.Button(organize_frame, text="Input", command=lambda: open_folder_if_exists(name_input_dir),
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=10).pack(side="left", padx=(0, 5))

        tk.Button(organize_frame, text="Organize and Rename", command=organize_and_rename_files,
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=20).pack(side="left", padx=(0, 5))

        tk.Button(organize_frame, text="Output", command=lambda: open_folder_if_exists(name_output_dir),
                activebackground=SELECTED_COLOR, bg=BUTTON_COLOR, fg=TEXT_COLOR,
                bd=0, relief="flat", width=10).pack(side="left")

        name_result_label = tk.Label(self.content_frame, text="", fg="lightgreen", bg=BG_COLOR, font=("Helvetica", 10))
        name_result_label.pack()
        
        border_bottom = tk.Frame(self.content_frame, bg=BORDER_COLOR, height=1)
        border_bottom.pack(fill="x")

if __name__ == "__main__":
    app = CustomApp()
    app.mainloop()
