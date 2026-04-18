# Arpeggiator Plugin Specification (NTS-3 Kaoss Pad)

## Overview
The Arpeggiator is a "Generic FX" unit for the KORG NTS-3 Kaoss Pad Kit. Unlike typical effects that process incoming audio, this unit acts as a performance-oriented **sound source**. It generates melodic arpeggios based on touch pad coordinates and internal tempo synchronization.

## Project Architecture
The plugin follows the standard NTS-3 logue SDK structure:
- **`header.c`**: Defines the unit as a `genericfx` type and sets default X/Y/Depth hardware mappings.
- **`effect.h`**: Contains the core logic for note generation, arpeggio sequencing, and DSP (Oscillators, Envelopes).
- **`unit.cc`**: Interfaces with the SDK callbacks (`unit_init`, `unit_render`, `unit_tempo_4ppqn_tick`, etc.).

## Functional Requirements
- **Performance Sound Source**: Audio generation triggered by pad touch.
- **Chord Support**: Generation of chords (Major, Minor, 7ths, etc.) based on a root pitch.
- **Tempo Synchronization**: Arpeggio steps are tied to the NTS-3's internal/external tempo via 16th-note ticks.
- **Playback Modes**: Support for Up, Down, Up-Down, and Random directions.
- **Waveform Selection**: Choice of classic oscillator shapes (Saw, Sine, Square).

## Parameter Specifications

The plugin utilizes 6 internal parameters, mapped to physical controls for real-time performance.

| Parameter | Control Mapping | Range | Description |
| :--- | :--- | :--- | :--- |
| **ROOT / X** | **X-Axis** | 0 - 1023 | Maps to the base pitch (Root Note). Total range is typically 2-3 octaves. |
| **CHORD / Y** | **Y-Axis** | 0 - 10 | Selects the Chord Type (e.g., Major, Minor, Sus4). |
| **GATE / DEPTH**| **DEPTH** Knob | 0 - 1023 | Controls the duration/sustain of each arpeggiated note. |
| **PATTERN** | Menu | 0 - 7 | Selects the playback rhythm (1/4, 1/8, 1/16, etc. including Triplets). |
| **MODE** | Menu | 0 - 4 | Arpeggio direction: 0=Up, 1=Down, 2=Up-Down, 3=Random, 4=Seq. |
| **RANGE** | Menu | 1 - 4 | Number of octaves the arpeggio spans. |
| **WAVE** | Menu | 0 - 2 | Oscillator type: 0=Saw, 1=Sine, 2=Square. |

## Technical Implementation Details

### 1. Pitch Logic (Internal Note Indexing)
While the NTS-3 does not "receive MIDI", it uses an internal note numbering system (0-127) for frequency calculation via the SDK's `osc_notehzf(note)`.
- **Root Pitch**: Calculated from the X-Axis coordinate (0-1023) mapped to a specific pitch range (e.g., C2 to C5).
- **Chord Offsets**: A lookup table defines the semitone offsets for the current chord type.
    - *Example (Major 7th)*: `{0, 4, 7, 11}`
- **Active Note List**: On each tick, the engine calculates the list of pitches by combining:
    `[Root] + [Chord Offset] + ([Current Octave] * 12)`

### 2. Arpeggio Engine (`tempo4ppqnTick`)
The sequence advances inside the `unit_tempo_4ppqn_tick(counter)` callback.
- **Clock Division**: The engine counts incoming 16th-note ticks (4 PPQN) and triggers a new note based on the **PATTERN** parameter.
- **Sequencer State**: Tracks the current index in the active note list and the current direction.
- **Retriggering**: If the touch pad is moved significantly (Root/Chord change), the arpeggio may optionally reset or update in real-time.

### 3. Sound Synthesis (`Process`)
The audio loop in `effect.h` implements a monophonic synth voice:
- **Oscillator**: Uses `osc_sawf`, `osc_sinf`, or `osc_sqrf` with the frequency derived from the current arpeggio note.
- **Envelope**: A simple AD (Attack/Decay) or gated envelope. The "Gate" time is modulated by the **DEPTH** control.
- **Smoothing**: Frequency changes are smoothed to prevent clicks during arpeggio steps.
- **Mixing**: As a "Generic FX" source, the unit provides a mix between the dry input and the generated arpeggio signal.

### 4. Chord Reference Table (Semitone Offsets)
| Index | Name | Offsets |
| :--- | :--- | :--- |
| 0 | Single | {0} |
| 1 | Major | {0, 4, 7} |
| 2 | Minor | {0, 3, 7} |
| 3 | Dom 7 | {0, 4, 7, 10} |
| 4 | Maj 7 | {0, 4, 7, 11} |
| 5 | Min 7 | {0, 3, 7, 10} |
| 6 | Sus 4 | {0, 5, 7} |
| 7 | Dim | {0, 3, 6} |
| 8 | Aug | {0, 4, 8} |

## Data Flow & Event Loop

1. **Touch Down**: `touchEvent` sets `is_active = true`, arpeggio starts from the first note.
2. **Tick Event**: `tempo4ppqnTick` increments the step counter. If the counter matches the division (e.g., every 2nd tick for 1/8 notes), the next pitch is calculated.
3. **Audio Render**:
    - The `Process` loop calculates the current sample value from the oscillator.
    - Applies a gain envelope based on the time since the last tick/event.
    - Outputs the signal to both Left/Right channels (Dual Mono).
4. **Touch Up**: `touchEvent` sets `is_active = false`, oscillator gain is faded out (simple release).

## Build Guide
Follow the same build procedure as other NTS-3 units:
```bash
./docker/run_cmd.sh build nts-3_kaoss/arpeggiator
```
The resulting `arpeggiator.nts3unit` can be loaded via the NTS-3 Librarian.