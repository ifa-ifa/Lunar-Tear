import os
import subprocess
import shutil

# --- Configuration ---
UNSEALED_VERSES_PATH = "UnsealedVerses.exe"
TEXCONV_PATH = "texconv.exe"
INPUT_PNG_DIR = "input_png"
RECONSTRUCTED_DDS_DIR = "reconstructed_dds"
OUTPUT_PACK_DIR = "output_pack"
ORIGINAL_PACK_DIR = "input_pack"

def main():
    print("--- Starting Reconstruction Process ---")

    for tool in [UNSEALED_VERSES_PATH, TEXCONV_PATH]:
        if not shutil.which(tool):
            print(f"FATAL ERROR: Required tool '{tool}' not found.")
            return

    os.makedirs(INPUT_PNG_DIR, exist_ok=True)
    os.makedirs(RECONSTRUCTED_DDS_DIR, exist_ok=True)
    os.makedirs(OUTPUT_PACK_DIR, exist_ok=True)

    print("Converting .png files to .dds using sidecar metadata files...")
    png_files = [f for f in os.listdir(INPUT_PNG_DIR) if f.endswith(".png")]
    
    if not png_files:
        print(f"Error: No .png files found in '{INPUT_PNG_DIR}' directory.")
        return

    for png_filename in png_files:
        png_path = os.path.join(INPUT_PNG_DIR, png_filename)
        meta_path = os.path.join(INPUT_PNG_DIR, os.path.splitext(png_filename)[0] + ".meta.txt")

        if not os.path.exists(meta_path):
            print(f"Warning: No sidecar file found for {png_filename}. Skipping.")
            continue
        
        with open(meta_path, 'r') as f:
            dds_format = f.read().strip()

        if not dds_format:
            print(f"Warning: Sidecar file for {png_filename} is empty. Skipping.")
            continue

        print(f"  - {png_filename} requires format: {dds_format}")

        # --- THIS IS THE FINAL FIX ---
        # Base command for texconv
        command = [
            TEXCONV_PATH,
            "-f", dds_format,
            "-o", RECONSTRUCTED_DDS_DIR,
            "-y",
            png_path
        ]

        # Conditionally add the sRGB input flag for color textures.
        # This tells texconv to correctly handle the gamma curve.
        if "_SRGB" in dds_format.upper():
            print("    - sRGB format detected. Applying gamma correction.")
            command.append("-srgbi")
        # --- END OF FIX ---

        try:
            subprocess.run(command, check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            print(f"    - FAILED to convert {png_filename} back to DDS: {e.stderr.strip()}")
            continue
        print(f"    - Successfully converted to {os.path.splitext(png_filename)[0] + '.dds'}")

    # --- (Patching process is unchanged) ---
    print("\nPatching the original PACK file...")
    pack_files = [f for f in os.listdir(ORIGINAL_PACK_DIR) if f.endswith(".xap")]
    if not pack_files:
        print(f"Error: No original .xap file found in '{ORIGINAL_PACK_DIR}' to patch.")
        return

    if not os.listdir(RECONSTRUCTED_DDS_DIR):
        print("Error: No DDS files were successfully reconstructed. Aborting patch process.")
        return

    original_pack_path = os.path.join(ORIGINAL_PACK_DIR, pack_files[0])
    output_pack_path = os.path.join(OUTPUT_PACK_DIR, "modded_" + pack_files[0])

    print(f"Patching '{original_pack_path}' with textures from '{RECONSTRUCTED_DDS_DIR}'...")
    try:
        subprocess.run([UNSEALED_VERSES_PATH, "pack-patch", output_pack_path, original_pack_path, RECONSTRUCTED_DDS_DIR], check=True)
    except subprocess.CalledProcessError:
        print(f"  - FAILED to patch the PACK file.")
        return

    print("\n--- Reconstruction Complete ---")
    print(f"Your modded PACK file has been created at: '{output_pack_path}'")

if __name__ == "__main__":
    main()