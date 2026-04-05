import os
import subprocess
import sys

script_dir = os.path.dirname(os.path.abspath(__file__))
tools_dir = os.path.join(script_dir, "tools")
vgmstream_cli_path = os.path.join(tools_dir, "vgmstream-cli")

input_dir = os.path.join(script_dir, "input", "wem")
output_dir = os.path.join(script_dir, "output", "wem_output")

def check_ffmpeg_installed():
    try:
        subprocess.run(["ffmpeg", "-version"], check=True, capture_output=True)
        print("FFmpeg is installed.")
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("WARNING: FFmpeg not found. vgmstream-cli might need it for some formats.", file=sys.stderr)

def ensure_output_dir():
    if not os.path.exists(output_dir):
        print(f"Creating output directory: {output_dir}")
        os.makedirs(output_dir)

def convert_files():
    print(f"Scanning input directory: {input_dir}")

    if not os.access(vgmstream_cli_path, os.X_OK):
        print(f"vgmstream-cli is not executable. Setting permissions...")
        try:
            os.chmod(vgmstream_cli_path, 0o755)
        except Exception as e:
            print(f"ERROR: Could not set executable permission for vgmstream-cli: {e}", file=sys.stderr)
            sys.exit(1)

    found_files = 0
    converted_files = 0

    for filename in os.listdir(input_dir):
        if filename.endswith(".wem"):
            found_files += 1

            input_file_path = os.path.join(input_dir, filename)

            output_filename = os.path.splitext(filename)[0] + ".wav"
            output_file_path = os.path.join(output_dir, output_filename)

            print(f"[{found_files}] Converting: {filename} -> {output_filename}")

            command = [
                vgmstream_cli_path,
                "-o",
                output_file_path,
                input_file_path
            ]

            try:
                subprocess.run(command, check=True, capture_output=True, text=True, cwd=tools_dir)
                converted_files += 1
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to convert {filename}.", file=sys.stderr)
                print(f"vgmstream-cli output (stderr):\n{e.stderr}\n", file=sys.stderr)
            except FileNotFoundError:
                 print(f"ERROR: '{vgmstream_cli_path}' not found. Check the path.", file=sys.stderr)
                 sys.exit(1)

    print("\n--- Process Complete ---")
    print(f"Found .wem files: {found_files}")
    print(f"Successfully converted: {converted_files}")

if __name__ == "__main__":
    check_ffmpeg_installed()
    ensure_output_dir()
    convert_files()
