# Polyphonic Synthesizer Project

A C++ polyphonic synthesizer capable of generating sound, processing MIDI input in real-time, and playing MIDI files. This project demonstrates various synthesis techniques including oscillators, filters, envelopes, LFOs, and effects.

## Features

*   **Polyphonic Voice Architecture:** Supports multiple simultaneous voices.
*   **Oscillators:** Multiple waveform options (Sine, Saw, Square, Triangle, Pulse, Additive).
    *   Pulse Width Modulation (PWM).
    *   Oscillator Sync.
    *   Cross-modulation (FM).
*   **Filters:** Multiple filter types (LPF24, LPF12, HPF12, BPF12, Notch) with resonance and key-following.
*   **Envelopes:** ADSR envelopes for amplitude and filter.
    *   Velocity sensitivity for envelopes.
*   **LFO:** Low-Frequency Oscillator for modulating various parameters (pitch, pulse width, filter cutoff).
*   **Modulation:**
    *   Polyphonic modulation matrix (Filter Env to Osc Freq/PW, Filter Env to Filter Cutoff, Osc B to Osc PW/Filter Cutoff).
    *   Modulation wheel assignable to LFO or Noise, targeting oscillator pitch, pulse width, or filter cutoff.
*   **Effects:**
    *   Built-in Reverb effect.
    *   Mixer Drive and Post Gain per voice.
*   **MIDI Control:**
    *   Real-time MIDI input for notes, pitch bend, and modulation wheel.
    *   MIDI file playback.
*   **Parameter Management:**
    *   Load synth parameters from a JSON file.
    *   Defaults to a basic strings sound if no JSON file is found.
*   **Analog Drift Simulation:** Optional pitch and pulse width drift for a more analog character.
*   **Unison Mode:** Stack voices for a thicker sound with detune and stereo spread.
*   **Glide/Portamento:** Smooth pitch transitions between notes.

## Dependencies

This project relies on the following external libraries:

*   **PortAudio:** For cross-platform audio I/O.
    *   Website: [http://www.portaudio.com/](http://www.portaudio.com/)
*   **RtMidi:** For cross-platform real-time MIDI input.
    *   Website: [https://www.music.mcgill.ca/~gary/rtmidi/](https://www.music.mcgill.ca/~gary/rtmidi/)
*   **Midifile (by Craig Stuart Sapp):** For reading and parsing Standard MIDI Files.
    *   GitHub: [https://github.com/craigsapp/midifile](https://github.com/craigsapp/midifile)
    *   License: Public Domain (or similar highly permissive license). Included in this repository for convenience.
*   **nlohmann/json:** For parsing JSON parameter files.
    *   GitHub: [https://github.com/nlohmann/json](https://github.com/nlohmann/json)
    *   License: MIT License. Included in this repository (single header file `include/nlohmann/json.hpp`).

## Building the Project

### Prerequisites

Ensure you have the following installed:

*   A C++17 compliant compiler (e.g., GCC, Clang).
*   Make.
*   **PortAudio development libraries:**
    *   macOS (Homebrew): `brew install portaudio`
    *   Linux (Debian/Ubuntu): `sudo apt-get install portaudio19-dev`
*   **RtMidi development libraries:**
    *   macOS (Homebrew): `brew install rtmidi`
    *   Linux (Debian/Ubuntu): `sudo apt-get install librtmidi-dev`

The Midifile and nlohmann/json libraries are included in this repository.

### Compilation

1.  Clone the repository:
    ```bash
    git clone <your_repository_url>
    cd <repository_directory_name>/synth
    ```
    If you cloned a version that uses Git submodules for Midifile, you would need:
    ```bash
    git clone --recurse-submodules <your_repository_url>
    cd <repository_directory_name>/synth
    ```

2.  Compile the project using Make:
    ```bash
    make
    ```
    This will create an executable named `synth` in the current directory.

    If you encounter issues, you might need to adjust include paths or library linking flags in the `Makefile` to match your system's configuration. For macOS, ensure RtMidi is linked with `CoreMIDI.framework`.

## Running the Synthesizer

The synthesizer executable is run from the `synth` directory.

```bash
./synth [path_to_params.json] [path_to_song.mid] [midi_input_port_number]

exsample: ./synth synth_params.json test_song.mid