# DAroom — VST3 / AU starter project

DAroom is an original JUCE-based algorithmic reverb plugin starter with the selected 653 × 184 px GUI reference embedded in the build. The interface uses the supplied artwork as its background and places interactive parameter controls and a 36-preset browser over the corresponding hit areas.

## Included
- C++17 + JUCE 8.0.6 project
- VST3, AU (macOS, universal x86_64 + arm64, ad-hoc signed) and Standalone formats
- Mix, Predelay, Decay, Size and reverb mode parameters
- Parameter state save/restore
- 36 factory presets across room, hall, plate, chamber and ambience spaces
- Individual editable `.drpreset` preset files in `Presets/` (embedded into the binary at build time, so the plug-in always ships exactly these presets)
- GitHub Actions workflow for macOS and Windows builds
- Exact GUI reference at `Assets/DAroom_UI_Reference.png`

## Important quality note
This is a functional first implementation, not a claim of parity with ValhallaRoom. It uses JUCE's built-in stereo reverb as a starting point plus a predelay buffer. Matching a mature commercial reverb requires substantial DSP design, listening tests, CPU optimization, parameter smoothing, and validation across sample rates and DAWs.

## Build locally
Install CMake, a C++ toolchain, and Git. Then:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On macOS, CMake builds VST3 and AU targets. AU is only available on Apple platforms.

## GitHub Actions
Push the project to GitHub. The workflow builds on macOS and Windows and uploads artifacts. For macOS AU validation, use a Mac runner; a Windows runner cannot build AU.

## Changelog

### 1.1.0 - bug-fix and build release
- **Fixed: the project did not compile.** `juce::dsp::Reverb` has no `setSampleRate()` or per-sample `processStereo(float&, float&)`; the DSP now uses `juce::Reverb` with its block-based `processStereo/processMono` API.
- **Fixed: mono operation.** Mono buses previously read the (silenced) second buffer channel as input; mono now feeds both reverb legs and is processed with `processMono`.
- **Fixed: mix smoothers** started at a hard-coded 26/74 split in `prepareToPlay`, causing an audible jump when a session or preset used a different mix; they now initialise from the current parameter value.
- **Fixed: `Decay` and `Mode` barely affected the sound** (the `modeSize` table was dead code). Room size, damping and stereo width are now derived from Size, Decay and Mode, and `getTailLengthSeconds()` reports the real tail (decay + predelay) instead of a constant 12 s.
- **Fixed: preset recall was not saved with the session.** The selected preset is now persisted in the plug-in state (`presetIndex`), restored on load, and switches to "Manual" when you move a control.
- **Fixed: GUI readouts drew on top of the values baked into the artwork** (double text). The static numbers, preset name and mode strip are now erased with sampled background colours and redrawn live; the active mode is highlighted correctly and the artwork's arrows step through presets.
- **Fixed: preset values were duplicated** in a hard-coded switch in the editor and in `Presets/*.drpreset`. The `.drpreset` JSON files are now the single source of truth (embedded as binary data).
- Robustness: lazy predelay allocation if a host processes audio before `prepareToPlay`, buffer-pointer based processing (no per-sample bounds-checked access), deprecated JUCE 8 `Font(float)` calls replaced with `FontOptions`.
- CI: macOS runner builds a universal (Intel + Apple Silicon) VST3/AU/Standalone, ad-hoc signs the bundles and runs `auval` validation; concurrent runs on the same branch cancel each other.
