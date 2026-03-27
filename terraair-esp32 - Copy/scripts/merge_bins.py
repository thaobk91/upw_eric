# Alt script for bat

import os
import subprocess
from pathlib import Path
import sys

def merge_bins(esptool_cmd, output_path, flash_mode, flash_freq, flash_size, bin_files):
    """
    Merges multiple binary files into a single binary using esptool's merge_bin command.
    """
    merge_command = [
        esptool_cmd,
        "merge_bin",
        "--output", str(output_path),
        "--flash_mode", flash_mode,
        "--flash_freq", flash_freq,
        "--flash_size", flash_size
    ]

    # Append address and file path pairs
    for address, file_path in bin_files:
        merge_command.extend([address, str(file_path)])

    print("Executing merge command:")
    print(' '.join(merge_command))
    try:
        subprocess.check_call(merge_command)
        print(f"Merged binary created at: {output_path}")
    except subprocess.CalledProcessError as e:
        print(f"Error during merging binaries: {e}")
        sys.exit(1)

def main():
    # Define project directories
    project_dir = Path(__file__).parent.resolve()
    build_version = "v1.0.0"  # Change this to your actual version directory
    build_dir = project_dir / "build" / build_version
    dist_dir = project_dir / "dist"

    # Create dist directory if it doesn't exist
    dist_dir.mkdir(parents=True, exist_ok=True)

    # Path to esptool. If installed via pip, it's assumed to be in PATH
    esptool_cmd = "esptool"  # Use "esptool.exe" if necessary

    # Define bin file paths with their respective memory addresses
    bin_files = [
        ("0x1000", build_dir / "AirMonitor_Feather.ino.bootloader.bin"),
        ("0x8000", build_dir / "AirMonitor_Feather.ino.partitions.bin"),
        ("0xe000", project_dir / "build" / "boot_app0.bin"),
        ("0x10000", build_dir / "AirMonitor_Feather.ino.bin"),
        ("0x670000", project_dir / "build" / "config.bin")
    ]

    # Verify all bin files exist
    for address, file_path in bin_files:
        if not file_path.is_file():
            print(f"Error: Binary file not found at {file_path}")
            sys.exit(1)

    # Define output merged binary path
    merged_bin_path = dist_dir / "merged.bin"

    # Merge binaries
    merge_bins(
        esptool_cmd=esptool_cmd,
        output_path=merged_bin_path,
        flash_mode="dio",
        flash_freq="80m",
        flash_size="8MB",
        bin_files=bin_files
    )

    print("Bin merging completed successfully.")

if __name__ == "__main__":
    main()
