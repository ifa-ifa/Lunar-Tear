
# Batch decompile scripts and fix encoding issues. Need to install luadec, run this from the same folder.

import sys
import os
import subprocess

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {os.path.basename(sys.argv[0])} path\\to\\folder\\with\\bytecode\\scripts")
        sys.exit(1)

    target_folder = sys.argv[1]

    if not os.path.isdir(target_folder):
        print(f'Error: Folder "{target_folder}" does not exist.')
        sys.exit(1)

    print(f'Processing folder: {target_folder}')
    processed_count = 0
    for filename in os.listdir(target_folder):
        if filename.lower().endswith('.lub'):
            full_path = os.path.join(target_folder, filename)
            output_file = os.path.splitext(full_path)[0] + '.lua'
            print(f'Processing {filename}...')

            try:
 
                result = subprocess.run(
                    ['luadec.exe', full_path],
                    capture_output=True,
                    check=True  
                )


                decompiled_code = result.stdout.decode('cp932')


                with open(output_file, 'w', encoding='utf-8') as outfile:
                    outfile.write(decompiled_code)
                
                processed_count += 1

            except subprocess.CalledProcessError as e:
                error_message = e.stderr.decode('cp932', errors='replace')
                print(f"Warning: Failed to process {filename} (return code {e.returncode}). Skipping.")
                print(f"  --> Error from luadec: {error_message.strip()}")
            except UnicodeDecodeError:
                print(f"Warning: Could not decode the output for {filename} as Japanese (Shift-JIS/CP932). Skipping.")
            except Exception as e:
                print(f"Warning: An unexpected error occurred while processing {filename}: {e}. Skipping.")

    print(f'\nDone. Processed {processed_count} file(s).')

if __name__ == "__main__":
    main()