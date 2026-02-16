# 54-Channel Audio Player

A multichannel audio playback system for the Allosphere, built with [allolib](https://github.com/AlloSphere-Research-Group/allolib).

## Features

- **Large File Support**: Streaming playback for files up to 2.5GB+ without loading into memory
- **Double Buffering**: Seamless audio playback with background pre-loading
- **Real-time File Switching**: Change audio files during playback
- **54-Channel Mapping**: Optimized for Allosphere speaker layout
- **Real-time Meters**: dB level monitoring for all channels
- **GUI Controls**: Full playback control with ImGui interface

## Streaming Implementation

For large audio files, the player uses an advanced **double buffering streaming system**:

- **Memory Efficient**: Only loads 2 chunks (~6MB) instead of entire file (2.5GB+)
- **Seamless Playback**: Background thread pre-loads audio chunks
- **Thread Safe**: Mutex-protected I/O with atomic buffer coordination
- **Reliable**: Graceful fallback to direct reading prevents dropouts

See [`streamingWAV.md`](streamingWAV.md) for detailed implementation documentation.

## Speaker Layout

The Allosphere has **54 speakers** arranged in three rings:

| Ring   | Speakers | Allosphere Channels | Elevation                  |
| ------ | -------- | ------------------- | -------------------------- |
| Upper  | 12       | 1-12                | Positive (above ear level) |
| Middle | 30       | 17-46               | Zero (ear level)           |
| Lower  | 12       | 49-60               | Negative (below ear level) |
| Sub    | 1        |                     | 48                         |

**Skipped channels**: 13-16, 47 (no physical speakers)

## Files

| File                 | Description                                    |
| -------------------- | ---------------------------------------------- |
| `mainplayer.hpp`     | Main application with GUI and audio playback   |
| `channelMapping.hpp` | Channel mapping configuration (file → speaker) |
| `streamingWAV.md`    | Streaming implementation documentation         |
| `CMakeLists.txt`     | CMake build configuration                      |
| `sourceAudio/`       | Directory for audio files                      |

---

## Usage

### 1. Build

```bash
cd 54ChanPlayer
mkdir -p build
cd build
cmake ..
cmake --build .
```

### 2. Add Audio Files

Place your 54-channel audio files (`.wav`, `.aiff`, `.flac`) in the `sourceAudio/` directory.

### 3. Run

```bash
./mainplayer
```

### 4. Select Audio File

Use the **dropdown menu** at the top of the GUI to switch between audio files. No rebuild required!

Click **↻ Refresh** to rescan the folder if you add new files while the app is running.

### GUI Controls

| Control           | Function                            |
| ----------------- | ----------------------------------- |
| **File Dropdown** | Select audio file from sourceAudio/ |
| **↻ Refresh**     | Rescan folder for new files         |
| **Play/Pause**    | Start or pause playback             |
| **Stop**          | Stop and reset to beginning         |
| **Rewind**        | Return to beginning                 |
| **Loop**          | Toggle looping                      |
| **Gain**          | Master volume (0.0 - 1.0)           |
| **Show Meters**   | Toggle dB meter display             |

### Supported Audio Formats

- `.wav` (recommended)
- `.aiff` / `.aif`
- `.flac`

---

## Channel Mapping

Audio file channels map to Allosphere outputs as follows:

```
File Ch 1-12  → Allo Ch 1-12   (Upper Ring)
File Ch 13-42 → Allo Ch 17-46  (Middle Ring)
File Ch 43-54 → Allo Ch 49-60  (Lower Ring)
File Ch 56 -> Allo Ch 48 (Sub)
```

To modify mappings, edit `channelMapping.hpp`.

---

## Requirements

- CMake 3.5+
- C++17 compiler
- allolib (included as submodule in parent directory)

## License

See parent project license.
