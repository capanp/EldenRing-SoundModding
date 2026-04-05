import os
import subprocess
import sys

script_dir = os.path.dirname(os.path.abspath(__file__))
input_dir = os.path.join(script_dir, "input", "mp3")
output_dir = os.path.join(script_dir, "output", "wav")

def check_ffmpeg_installed():
    print("Checking for FFmpeg...")
    try:
        subprocess.run(["ffmpeg", "-version"], check=True, capture_output=True, text=True)
        print("FFmpeg is installed.")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("ERROR: FFmpeg is not installed or not in system PATH.", file=sys.stderr)
        print("Please install FFmpeg to use this script.", file=sys.stderr)
        return False

def ensure_output_dir():
    if not os.path.exists(output_dir):
        print(f"Creating output directory: {output_dir}")
        os.makedirs(output_dir)

def convert_files():
    print(f"Scanning input directory: {input_dir}")

    found_files = 0
    converted_files = 0

    for filename in os.listdir(input_dir):
        if filename.endswith(".mp3"):
            found_files += 1

            input_file_path = os.path.join(input_dir, filename)

            output_filename = os.path.splitext(filename)[0] + ".wav"
            output_file_path = os.path.join(output_dir, output_filename)

            print(f"[{found_files}] Converting: {filename} -> {output_filename}")

            command = [
                "ffmpeg",
                "-i",
                input_file_path,
                "-y",  # Automatically overwrite output files
                output_file_path
            ]

            try:
                subprocess.run(command, check=True, capture_output=True, text=True)
                converted_files += 1
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to convert {filename}.", file=sys.stderr)
                print(f"FFmpeg output (stderr):\n{e.stderr}\n", file=sys.stderr)

    print("\n--- Process Complete ---")
    print(f"Found .mp3 files: {found_files}")
    print(f"Successfully converted: {converted_files}")

if __name__ == "__main__":
    if check_ffmpeg_installed():
        ensure_output_dir()
        convert_files()
    else:
        sys.exit(1)
