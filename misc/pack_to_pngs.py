import os
import subprocess
from pngmeta import PngMeta

UNSEALED_VERSES_PATH = "UnsealedVerses.exe"
TEXCONV_PATH = "texconv.exe"
INPUT_PACK_DIR = "input_pack"
EXTRACTED_RTEX_DIR = "extracted_rtex"
EXTRACTED_DDS_DIR = "extracted_dds"
OUTPUT_PNG_DIR = "output_png"

def main():

    os.makedirs(INPUT_PACK_DIR, exist_ok=True)
    os.makedirs(EXTRACTED_RTEX_DIR, exist_ok=True)
    os.makedirs(EXTRACTED_DDS_DIR, exist_ok=True)
    os.makedirs(OUTPUT_PNG_DIR, exist_ok=True)

    pack_files = [f for f in os.listdir(INPUT_PACK_DIR) if f.endswith(".xap")]
    if not pack_files:
        print(f"Error: No .xap file found in '{INPUT_PACK_DIR}' directory.")
        return

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
                subprocess.run([UNSEALED_VERSES_PATH, "rtex-to-dds", dds_path, rtex_path], check=True)
            except subprocess.CalledProcessError as e:
                print(f"  - Could not convert {rtex_filename}. It might not be a texture. Skipping.")


    print("\nConverting .dds to .png and embedding metadata...")
    for dds_filename in os.listdir(EXTRACTED_DDS_DIR):
        if dds_filename.endswith(".dds"):
            dds_path = os.path.join(EXTRACTED_DDS_DIR, dds_filename)
            png_filename = os.path.splitext(dds_filename)[0] + ".png"
            png_path = os.path.join(OUTPUT_PNG_DIR, png_filename)

            # Get DDS format using texconv
            result = subprocess.run([TEXCONV_PATH, "-info", dds_path], capture_output=True, text=True)
            dds_format = ""
            for line in result.stdout.splitlines():
                if "format = DXGI_FORMAT_" in line:
                    dds_format = line.split("DXGI_FORMAT_")[1].strip().split(" ")[0]
                    break

            if not dds_format:
                print(f"Warning: Could not determine format for {dds_filename}. Skipping.")
                continue

            print(f"  - {dds_filename} format is {dds_format}")

            subprocess.run([TEXCONV_PATH, "-ft", "png", "-o", OUTPUT_PNG_DIR, dds_path], check=True)

            meta = PngMeta()
            meta.load(png_path)
            meta.add_text("DDSFormat", dds_format)
            meta.save(png_path)
            print(f"    - Converted to {png_filename} and saved metadata.")

    print(f"PNG files with metadata are in the '{OUTPUT_PNG_DIR}' directory.")

if __name__ == "__main__":
    main()