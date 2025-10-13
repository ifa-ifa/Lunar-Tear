import os
import subprocess
import shutil

# --- Configuration ---
UNSEALED_VERSES_PATH = "UnsealedVerses.exe"
TEXCONV_PATH = "texconv.exe"
TEXDIAG_PATH = "texdiag.exe"
INPUT_PACK_DIR = "input_pack"
EXTRACTED_RTEX_DIR = "extracted_rtex"
EXTRACTED_DDS_DIR = "extracted_dds"
OUTPUT_PNG_DIR = "output_png"

def main():
    print("--- Starting Deconstruction Process ---")

    for tool in [UNSEALED_VERSES_PATH, TEXCONV_PATH, TEXDIAG_PATH]:
        if not shutil.which(tool):
            print(f"FATAL ERROR: Required tool '{tool}' not found.")
            return

    os.makedirs(OUTPUT_PNG_DIR, exist_ok=True)
    # ... (rest of setup code is the same)

    # --- (Steps 1 & 2 are the same as before) ---
    print("\nConverting .dds to .png and creating metadata sidecar files...")
    for dds_filename in os.listdir(EXTRACTED_DDS_DIR):
        if dds_filename.endswith(".dds"):
            dds_path = os.path.join(EXTRACTED_DDS_DIR, dds_filename)
            png_filename = os.path.splitext(dds_filename)[0] + ".png"
            png_path = os.path.join(OUTPUT_PNG_DIR, png_filename)
            meta_path = os.path.join(OUTPUT_PNG_DIR, os.path.splitext(dds_filename)[0] + ".meta.txt")

            result = subprocess.run([TEXDIAG_PATH, "info", dds_path], capture_output=True, text=True)
            
            dds_format = ""
            for line in result.stdout.splitlines():
                if "format =" in line:
                    dds_format = line.split('=')[1].strip()
                    break

            if not dds_format:
                print(f"Warning: Could not determine format for {dds_filename}. Skipping.")
                continue

            print(f"  - {dds_filename} format is {dds_format}")

            command = [TEXCONV_PATH, "-ft", "png", "-o", OUTPUT_PNG_DIR, "-y", dds_path]
            if dds_format == "BC5_UNORM":
                command.extend(["-f", "R8G8B8A8_UNORM"])
            
            try:
                subprocess.run(command, check=True, capture_output=True)
            except subprocess.CalledProcessError as e:
                print(f"    - FAILED: texconv.exe error on {dds_filename}: {e.stderr.decode('utf-8', errors='ignore').strip()}")
                continue

            # --- NEW SIDECAR LOGIC ---
            with open(meta_path, 'w') as f:
                f.write(dds_format)
            print(f"    - Converted to {png_filename} and saved metadata to {os.path.basename(meta_path)}.")
            # --- END NEW LOGIC ---

    print("\n--- Deconstruction Complete ---")
    print(f"PNG files and .meta.txt files are in the '{OUTPUT_PNG_DIR}' directory.")

# Boilerplate main call
if __name__ == "__main__":
    # Re-add the other parts of main if they were removed
    os.makedirs(INPUT_PACK_DIR, exist_ok=True)
    os.makedirs(EXTRACTED_RTEX_DIR, exist_ok=True)
    os.makedirs(EXTRACTED_DDS_DIR, exist_ok=True)
    
    pack_files = [f for f in os.listdir(INPUT_PACK_DIR) if f.endswith(".xap")]
    if not pack_files:
        print(f"Error: No .xap file found in '{INPUT_PACK_DIR}' directory.")
    else:
        input_pack_path = os.path.join(INPUT_PACK_DIR, pack_files[0])
        print(f"Found PACK file: {input_pack_path}")
        print(f"Unpacking '{input_pack_path}' to '{EXTRACTED_RTEX_DIR}'...")
        subprocess.run([UNSEALED_VERSES_PATH, "unpack", input_pack_path, EXTRACTED_RTEX_DIR], check=True)
        
        print("\nConverting .rtex files to .dds...")
        for rtex_filename in os.listdir(EXTRACTED_RTEX_DIR):
            if rtex_filename.endswith(".rtex"):
                rtex_path = os.path.join(EXTRACTED_RTEX_DIR, rtex_filename)
                dds_filename = os.path.splitext(rtex_filename)[0] + ".dds"
                dds_path = os.path.join(EXTRACTED_DDS_DIR, dds_filename)
                try:
                    subprocess.run([UNSEALED_VERSES_PATH, "rtex-to-dds", dds_path, rtex_path], check=True, capture_output=True)
                except subprocess.CalledProcessError:
                    print(f"  - Could not convert {rtex_filename}. Skipping.")
        main()