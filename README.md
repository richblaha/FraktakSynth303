# FraktalSynth 303

> Subtractive synthesizer with chaotic, fractal-inspired sound design — built with JUCE.

A polyphonic VST3/AU plugin combining classic 303-style ladder filter architecture with experimental oscillator modes: Lorenz attractors, FM feedback, granular textures, and morphable wavetables.

---

## Features

### Oscillator
- **5 waveforms:** Lorenz attractor, FM Feedback, Granular, Saw, Square
- **MORPH** – continuous morphing between all 5 waveforms with equal-power crossfade
- **PWM** – pulse width modulation for the Square waveform
- **Granular mix** – two independent granular layers (slow drift + fast glitch)

### Filter
- **Moog-style 4-pole ladder filter** (Huovilainen model)
- Cutoff: 30–17000 Hz with musical response curve
- **Resonance** up to self-oscillation
- **Drive** – pre-filter saturation (tanh waveshaper)
- **Filter Env** – ADSR-driven cutoff modulation
- **LFO** – rate 0.05–12 Hz with variable depth

### Envelope
- Full **ADSR** with musical ranges (attack 1ms–3s, release up to 4s)

### FX / Live
- **SMEAR (Chaos)** – Lorenz-based pitch/phase smearing
- **FRAKTAL** – fractal wavefolder / harmonic distortion
- **STUTTER** – rhythmic buffer stutter, Free or DAW-synced
  - Divisions: 1/4 to 1/128, including triplets (T), quintuplets (Q), septuplets (S)
- **PHASE DEST** – phase destabilization
- **TIME FOLD** – time-domain folding
- **MORPH AUTO** – automated morph modulation

---

## System Requirements

| | Minimum |
|---|---|
| OS | macOS (tested) |
| CPU | Intel/AMD x64, Apple Silicon |
| Format | VST3, AU |
| DAW | Any VST3/AU compatible host |

## Platform Notes

The plugin has been tested on macOS.

The codebase is cross-platform (JUCE) and can be built for Windows and Linux using the JUCE toolchain, but prebuilt binaries are currently only provided for macOS.
---

## Building from Source

This plugin requires **JUCE 7** or later.

```bash
git clone https://github.com/richblaha/FraktakSynth303
cd FraktakSynth303
# Open FraktalSynth303.jucer in Projucer, export to your IDE, then build
```

Dependencies: JUCE only. No third-party libraries.

---

## Presets (Reaper)

FraktalSynth 303 does not yet include an internal preset system.  
Presets can be saved and loaded using your DAW.

### Loading included presets (.RPL) in Reaper

1. Load FraktalSynth 303 in Reaper  
2. Open the plugin window  
3. Click the **"+" (preset menu)** at the top  
4. Select **Import preset library...**  
5. Choose the included `.RPL` file  

The presets will now appear in the preset menu.

Note: Presets are managed by Reaper, not by the plugin itself.

---
## License

This project is licensed under the **GNU General Public License v3.0**.

You are free to use, study, modify and redistribute this software under the terms of the GPL v3. If you distribute compiled binaries, you must make the source code available under the same license.

Full license text: https://www.gnu.org/licenses/gpl-3.0.txt

Built with [JUCE](https://juce.com) (GPL v3 tier).

---

## Source Code

https://github.com/richblaha/FraktakSynth303

---

## Support / Contact

Issues and pull requests welcome via GitHub.
