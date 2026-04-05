import os
import subprocess
import sys
import math

script_dir = os.path.dirname(os.path.abspath(__file__))
input_dir = os.path.join(script_dir, "output", "wem_output")
output_file = os.path.join(script_dir, "combined_audio.mp3")
list_file = os.path.join(script_dir, "temp_file_list.txt")

# 10MB limiti (Güvenli olması için 9.8MB kullanalım)
MAX_SIZE_BYTES = 9.8 * 1024 * 1024
TARGET_BITRATE_K = 192 # Birleştirme ve bölmede kullanılacak kalite

def check_ffmpeg_installed():
    print("Checking for FFmpeg and FFprobe...")
    try:
        subprocess.run(["ffmpeg", "-version"], check=True, capture_output=True, text=True)
        subprocess.run(["ffprobe", "-version"], check=True, capture_output=True, text=True)
        print("FFmpeg and FFprobe are installed.")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("ERROR: FFmpeg or FFprobe is not installed or not in system PATH.", file=sys.stderr)
        return False

def split_if_needed(combined_file_path):
    if not os.path.exists(combined_file_path):
        return

    try:
        file_size = os.path.getsize(combined_file_path)
    except FileNotFoundError:
        print(f"ERROR: Combined file not found, cannot split.", file=sys.stderr)
        return

    if file_size <= MAX_SIZE_BYTES:
        print(f"Combined file is {(file_size / (1024*1024)):.2f}MB. No splitting required.")
        return

    print(f"File is {(file_size / (1024*1024)):.2f}MB. Splitting into chunks smaller than {MAX_SIZE_BYTES / (1024*1024):.2f}MB...")

    # 1. Toplam süreyi al
    try:
        probe_cmd = [
            "ffprobe", "-v", "error", "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1", combined_file_path
        ]
        result = subprocess.run(probe_cmd, check=True, capture_output=True, text=True)
        total_duration_sec = float(result.stdout.strip())
    except Exception as e:
        print(f"ERROR: Could not get duration of combined file: {e}", file=sys.stderr)
        return

    # 2. Kaç parçaya bölüneceğini hesapla
    num_chunks = math.ceil(file_size / MAX_SIZE_BYTES)
    duration_per_chunk_sec = total_duration_sec / num_chunks

    print(f"Total duration: {total_duration_sec:.2f}s. Splitting into {num_chunks} parts.")

    # 3. Dosyaları böl
    base_name = os.path.splitext(combined_file_path)[0]
    try:
        for i in range(num_chunks):
            start_time = i * duration_per_chunk_sec
            output_part_name = f"{base_name}_part{i+1}.mp3"

            print(f"Creating: {output_part_name} (start: {start_time:.2f}s, duration: {duration_per_chunk_sec:.2f}s)")

            split_cmd = [
                "ffmpeg",
                "-i", combined_file_path,
                "-ss", str(start_time),
                "-t", str(duration_per_chunk_sec),
                "-b:a", f"{TARGET_BITRATE_K}k",
                "-y",
                output_part_name
            ]
            subprocess.run(split_cmd, check=True, capture_output=True, text=True)

        print(f"Splitting complete. Deleting original file: {combined_file_path}")
        os.remove(combined_file_path)

    except subprocess.CalledProcessError as e:
        print(f"ERROR: FFmpeg failed during splitting.", file=sys.stderr)
        print(f"FFmpeg output (stderr):\n{e.stderr}\n", file=sys.stderr)
    except Exception as e:
        print(f"ERROR during splitting process: {e}", file=sys.stderr)


def combine_files():
    if not os.path.exists(input_dir):
        print(f"ERROR: Input directory not found: {input_dir}", file=sys.stderr)
        sys.exit(1)

    wav_files = []
    for filename in sorted(os.listdir(input_dir)):
        if filename.endswith(".wav"):
            wav_files.append(os.path.join(input_dir, filename))

    if not wav_files:
        print(f"No .wav files found in {input_dir}. Exiting.", file=sys.stderr)
        sys.exit(0)

    print(f"Found {len(wav_files)} .wav files to combine.")

    try:
        with open(list_file, 'w', encoding='utf-8') as f:
            for file_path in wav_files:
                f.write(f"file '{file_path}'\n")
    except IOError as e:
        print(f"ERROR: Could not write temporary list file: {e}", file=sys.stderr)
        sys.exit(1)

    print("Starting combination process with FFmpeg...")

    command = [
        "ffmpeg",
        "-f", "concat",
        "-safe", "0",
        "-i", list_file,
        "-b:a", f"{TARGET_BITRATE_K}k",
        "-y",
        output_file
    ]

    try:
        subprocess.run(command, check=True, capture_output=True, text=True)
        print(f"Successfully combined audio to: {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"ERROR: FFmpeg failed to combine files.", file=sys.stderr)
        print(f"FFmpeg output (stderr):\n{e.stderr}\n", file=sys.stderr)
    finally:
        if os.path.exists(list_file):
            os.remove(list_file)
            print("Cleaned up temporary file.")

    # Birleştirmeden hemen sonra bölme kontrolünü yap
    split_if_needed(output_file)

if __name__ == "__main__":
    if check_ffmpeg_installed():
        combine_files()
    else:
        sys.exit(1)
