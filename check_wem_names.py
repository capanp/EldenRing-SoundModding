from collections import defaultdict

CUTSCENE_FILE = "./cutscene.md"

def extract_wem_ids(filepath):
    wem_map = defaultdict(list)

    with open(filepath, 'r', encoding='utf-8') as f:
        for lineno, line in enumerate(f, start=1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            try:
                parts = line.rsplit(",", 2)
                wem_id = parts[-2].strip()
                if wem_id.isdigit():
                    wem_map[wem_id].append((lineno, line))
            except Exception as e:
                print(f"Warning: Could not parse line {lineno}: {line} ({e})")

    return wem_map

def print_duplicates(wem_map):
    duplicates = {k: v for k, v in wem_map.items() if len(v) > 1}
    if not duplicates:
        print("No duplicate WEM IDs found.")
        return

    print("Duplicate WEM IDs found:\n")
    for wem_id, lines in duplicates.items():
        print(f"WEM ID: {wem_id}")
        for lineno, line in lines:
            print(f"  Line {lineno}: {line}")
        print()

def main():
    wem_map = extract_wem_ids(CUTSCENE_FILE)
    print_duplicates(wem_map)

if __name__ == "__main__":
    main()
