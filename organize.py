import os
import shutil

def organize_and_rename_files():
    # Define paths
    input_dir = 'input/organize'
    output_dir = 'output/organize'
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Process each file in the input directory
    for filename in os.listdir(input_dir):
        # Skip directories if any
        if os.path.isdir(os.path.join(input_dir, filename)):
            continue
            
        # Extract first two characters for subdirectory
        subdir_name = filename[:2]
        subdir_path = os.path.join(output_dir, subdir_name)
        
        # Create subdirectory if it doesn't exist
        os.makedirs(subdir_path, exist_ok=True)
        
        # Split filename at underscore and take the first part
        new_filename = filename.split('_')[0] + os.path.splitext(filename)[1]
        
        # Full paths for source and destination
        src_path = os.path.join(input_dir, filename)
        dest_path = os.path.join(subdir_path, new_filename)
        
        # Copy and rename the file
        shutil.copy2(src_path, dest_path)
        print(f"Processed: {filename} -> {os.path.join(subdir_name, new_filename)}")

if __name__ == "__main__":
    organize_and_rename_files()
    print("File organization and renaming completed!")