import re
import sys
import os
import glob

def batch_convert_wavetables(input_folder, output_path, table_size=256):
    print(f"Scanning for .h files in folder: '{input_folder}'...")

    if not os.path.exists(input_folder):
        print(f"Error: Folder '{input_folder}' not found.")
        print(f"Please create a folder named '{input_folder}' and put all your .h files inside it.")
        sys.exit(1)

    # Find all .h files in the target folder
    h_files = glob.glob(os.path.join(input_folder, "*.h"))

    if not h_files:
        print(f"Error: No .h files found in '{input_folder}'.")
        sys.exit(1)

    print(f"Found {len(h_files)} files. Extracting arrays...")

    # Regex to match the contents between { and }
    pattern = re.compile(r'=\s*\{([^}]+)\};')
    all_float_tables = []

    for file_path in h_files:
        with open(file_path, 'r') as f:
            raw_text = f.read()

        matches = pattern.findall(raw_text)

        for match in matches:
            # Extract numbers, strip whitespace and commas
            str_values = match.replace('\n', '').split(',')
            int_values = [int(v.strip()) for v in str_values if v.strip()]

            if not int_values:
                continue

            # Enforce strict table size (truncate or pad)
            if len(int_values) > table_size:
                int_values = int_values[:table_size]
            elif len(int_values) < table_size:
                int_values += [0] * (table_size - len(int_values))

            # Convert int16 (-32768 to 32767) to normalized float (-1.0f to 1.0f)
            float_values = [f"{val / 32768.0:.6f}f" for val in int_values]
            all_float_tables.append(float_values)

    if not all_float_tables:
        print("Error: Files were found, but no valid arrays were extracted.")
        sys.exit(1)

    print(f"Successfully extracted {len(all_float_tables)} total wavetables. Writing to C++ header...")

    # Write the unified C++ Header
    with open(output_path, 'w') as f:
        f.write("/**\n")
        f.write(" * @file wavetables.h\n")
        f.write(f" * Auto-generated {len(all_float_tables)} wavetables for ScrutaAstri\n")
        f.write(" */\n\n")
        f.write("#pragma once\n\n")

        f.write(f"constexpr int NUM_WAVETABLES = {len(all_float_tables)};\n")
        f.write(f"constexpr int WAVETABLE_SIZE = {table_size};\n\n")

        f.write("const float SCRUTAASTRI_WAVETABLES[NUM_WAVETABLES][WAVETABLE_SIZE] = {\n")

        for i, table in enumerate(all_float_tables):
            f.write(f"    // Wavetable {i}\n")
            f.write("    {\n        ")

            # Format nicely in rows of 8 numbers
            for j in range(0, len(table), 8):
                chunk = table[j:j+8]
                f.write(", ".join(chunk))
                if j + 8 < len(table):
                    f.write(",\n        ")

            f.write("\n    }")
            if i < len(all_float_tables) - 1:
                f.write(",")
            f.write("\n\n")

        f.write("};\n")

    print(f"Success! Generated '{output_path}'.")

if __name__ == "__main__":
    # Folder containing all your raw .h files
    INPUT_FOLDER = "raw_tables"
    # The single unified output file
    OUTPUT_FILE = "wavetables.h"

    batch_convert_wavetables(INPUT_FOLDER, OUTPUT_FILE)