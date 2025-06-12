import os
import shutil
from datetime import datetime

INPUT_DIR = "./input/name"
OUTPUT_DIR = "./output/name"
CUTSCENE_FILE = "./cutscene.md"

def parse_cutscene_lines(filepath):
    wem_ids = []

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            try:
                parts = line.rsplit(",", 2)
                wem_id = parts[-2].strip()

                if wem_id.lower() == "unkwem":
                    line_id = line.split(",", 1)[0]
                    wem_ids.append(f"unkwem-{line_id}")
                elif wem_id.isdigit():
                    wem_ids.append(f"{wem_id}")
                else:
                    raise ValueError(f"Invalid WEM ID: {wem_id} in line: {line}")
            except Exception as e:
                raise ValueError(f"Malformed line: {line}\n{e}")

    return wem_ids

def get_sorted_mp3_files(input_dir):
    mp3_files = [
        f for f in os.listdir(input_dir)
        if f.lower().endswith(".mp3") and os.path.isfile(os.path.join(input_dir, f))
    ]

    mp3_files.sort(key=lambda f: os.path.getmtime(os.path.join(input_dir, f)))
    return mp3_files

def rename_and_copy_files(wem_ids, mp3_files, input_dir, output_dir):
    os.makedirs(output_dir, exist_ok=True)

    for wem_id, mp3_file in zip(wem_ids, mp3_files):
        src_path = os.path.join(input_dir, mp3_file)
        dst_path = os.path.join(output_dir, f"{wem_id}.mp3")
        shutil.copy2(src_path, dst_path)
        print(f"Renamed {mp3_file} -> {wem_id}.mp3")

def main():
    try:
        wem_ids = parse_cutscene_lines(CUTSCENE_FILE)
        mp3_files = get_sorted_mp3_files(INPUT_DIR)

        if len(wem_ids) != len(mp3_files):
            print(f"Error: mismatch between line count ({len(wem_ids)}) and mp3 file count ({len(mp3_files)}).")
            return

        rename_and_copy_files(wem_ids, mp3_files, INPUT_DIR, OUTPUT_DIR)
        print("All files renamed and copied successfully.")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
