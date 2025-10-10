import os
import subprocess
from pngmeta import PngMeta

UNSEALED_VERSES_PATH = "UnsealedVerses.exe"
TEXCONV_PATH = "texconv.exe"
INPUT_PNG_DIR = "input_png"
RECONSTRUCTED_DDS_DIR = "reconstructed_dds"
OUTPUT_PACK_DIR = "output_pack"
ORIGINAL_PACK_DIR = "input_pack" # To get the original pack for patching

def main():

    os.makedirs(INPUT_PNG_DIR, exist_ok=True)
    os.makedirs(RECONSTRUCTED_DDS_DIR, exist_ok=True)
    os.makedirs(OUTPUT_PACK_DIR, exist_ok=True)

    print("Converting .png files to .dds using embedded metadata...")
    png_files = [f for f in os.listdir(INPUT_PNG_DIR) if f.endswith(".png")]
    
    if not png_files:
        print(f"Error: No .png files found in '{INPUT_PNG_DIR}' directory.")
        return

    for png_filename in png_files:
        png_path = os.path.join(INPUT_PNG_DIR, png_filename)
        dds_filename = os.path.splitext(png_filename)[0] + ".dds"
        dds_path = os.path.join(RECONSTRUCTED_DDS_DIR, dds_filename)

        meta = PngMeta()
        try:
            meta.load(png_path)
            dds_format_tuple = meta.get_text("DDSFormat")
        except Exception as e:
            print(f"Could not read metadata from {png_filename}. Error: {e}. Skipping.")
            continue

        if not dds_format_tuple:
            print(f"Warning: No DDSFormat metadata found in {png_filename}. Skipping.")
            continue

        dds_format = dds_format_tuple[1]
        print(f"  - {png_filename} format is {dds_format}")

        try:
            subprocess.run([TEXCONV_PATH, "-f", dds_format, "-o", RECONSTRUCTED_DDS_DIR, "-y", png_path], check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            print(f"    - FAILED to convert {png_filename} back to DDS.")
            print(f"    - Texconv Error: {e.stderr}")
            continue
        print(f"    - Successfully converted to {dds_filename}")


    print("\nPatching the original PACK file...")
    pack_files = [f for f in os.listdir(ORIGINAL_PACK_DIR) if f.endswith(".xap")]
    if not pack_files:
        print(f"Error: No original .xap file found in '{ORIGINAL_PACK_DIR}' to patch.")
        return

    original_pack_path = os.path.join(ORIGINAL_PACK_DIR, pack_files[0])
    output_pack_path = os.path.join(OUTPUT_PACK_DIR, "modded_" + pack_files[0])

    print(f"Patching '{original_pack_path}' to create '{output_pack_path}'...")
    try:
        subprocess.run([UNSEALED_VERSES_PATH, "pack-patch", output_pack_path, original_pack_path, RECONSTRUCTED_DDS_DIR], check=True)
    except subprocess.CalledProcessError as e:
        print(f"  - FAILED to patch the PACK file.")
        return

    print(f"Patched PACK file is in the '{OUTPUT_PACK_DIR}' directory.")

if __name__ == "__main__":
    main()