# damage-detector
Windows-exclusive tool for searching zero-chunks inside files.


## Features

* **Deep Video Analysis**: Supports over 25 popular video extensions (including MP4, MKV, AVI, MOV, TS, and more). Non-video files are automatically skipped.
* **Zero-Chunk Detection**: Locates continuous sequences of `0x00` bytes, which often indicate corrupted sectors, interrupted writes, or dropped frames.
* **Automatic Shortcut Generation**: Generates a standard Windows shortcut (`.lnk`) inside a dedicated `Shortcuts/` folder for every corrupted file found. Files are prefixed with a 4-character random alphanumeric string to prevent naming collisions.
* **Recursive Directory Scanning**: Seamlessly crawls entire directory trees, safely skipping files with restricted system permissions.
* **Real-Time CLI Feedback**: Displays live scanning metrics (current file index, chunk processing speed in ms, and cumulative execution time). Outputs precise zero-chunk offsets and lengths in both human-readable and hexadecimal formats.


## Installation

Executable can be compiled via CMAKE

```cmd
  cmake -B build
  cmake --build build --config Release
```

## Usage
Run the utility from your command prompt or terminal using the following syntax:
```DOS
damage-detector.exe <file_or_folder> [min_size] [ignore_threshold] [buffer_size]
```
Arguments:
- file_or_folder (Required): Absolute or relative path to the specific video file or directory you want to scan.
- min_size (Optional): The minimum continuous length of zero bytes (in bytes) required to flag a chunk as corrupted.
    - Default: 50000 bytes.
    - Smart Threshold: Setting this argument explicitly to 1 forces the program to automatically calculate a dynamic threshold equal to 5% of the total file size.
- ignore_threshold (Optional): The byte offset skipped at both the start and end of the file to preserve container headers.
    - Default: 1000000 bytes (1 MB).
- buffer_size (Optional): Internal read stream buffer capacity in bytes.
    - Default: 104857 bytes (~100 KB).
