# Thudi v1 (R4N) – install, use and build

Thudi (துடி) is a 16-pad drum synthesizer: no samples, every pad is its own synth voice.

- **Windows** (10 or 11): VST3, 64-bit. Developed and tested in FL Studio; works in any VST3 host.

Downloads: `Thudi-Setup-Windows.exe` (or `Thudi-VST3-Windows.zip`), from the
[latest release](https://github.com/R4N-WAV/thudi/releases/tag/latest) on GitHub.

## Install on Windows
**With the setup wizard (easiest):**
1. Close your music app, then run **Thudi-Setup-Windows.exe**.
2. Thudi isn't code-signed yet, so Windows may show *Windows protected your PC*: click **More info** → **Run anyway**. Allow the admin prompt (the VST3 folder is a system folder).
3. Click through the wizard. It installs `Thudi.vst3` into `C:\Program Files\Common Files\VST3` (you can choose another VST3 folder), replaces an older version cleanly and adds an uninstaller (**Settings → Apps → Thudi**). Your own presets in Documents are never touched.
4. In your host, rescan plugins (FL Studio: **Options → Manage plugins → Find installed plugins**) and load **Thudi** (listed under **R4N**) as an instrument in the Channel rack.

**By hand, with the zip:**
1. Right-click the zip → **Properties** → tick **Unblock** → **OK**. Unzip it.
2. Copy the folder **`Thudi.vst3`** into `C:\Program Files\Common Files\VST3`.
3. Rescan plugins in your host, as above.

## Use
- **Click a pad** to play it (higher on the pad = harder hit) and select it for editing. The red LED marks the selected pad.
- **Right-click a pad**: Edit sound, Rename, Copy / Paste / Paste sound only, Swap with, Copy to, Model (editor pages), Init as,
  Load / Save pad preset, Randomize, Choke group, Output, Link (trigger together), Play mode, play chromatically from the keyboard, MIDI learn.
- **The 8 knobs** under the LCD edit the page shown on the LCD. **SOFT KEY 1 / 2** step through the pages
  (each model has its own sound pages, plus CHAR, STEREO and PLAY). Double-click a knob for its default.
- **SOFT KEY 3 (BROWSE)**: kits and pad presets. Turn the **DATA ENTRY** wheel (or scroll) and press **ENTER** to load.
  When editing, the wheel fine-tunes the highlighted value.
- **SOFT KEY 4 (SAVE)**: save the kit or the selected pad. Your presets live in `Documents\R4N\Thudi\` (Kits, Pads\<category>).
- **MASTER**, **BUS DRIVE** and **OVERSAMPLE** (1x / 2x / 4x) are on the left.

## MIDI
Pads 1–16 play from C1–D#2 (notes 36–51); the bottom row is pads 1–4, as on an MPC. Any pad can learn another note.
In the RHYNO kit the KICK (pad 1) triggers the 808 (pad 2) through Link, and the 808 is in **Gate** mode: the note length
in the piano roll decides how long it rings (that's the bounce). Notes that aren't pad notes play the 808 chromatically, with glide.

## Separate outputs
Set a pad's **Output** (right-click → Output). In FL Studio enable the extra outputs in the plugin's wrapper
(gear icon → Processing) and route them to mixer tracks. Pads on Main, or on an output the host hasn't enabled, play through Main.

## Build
`cmake -S . -B build && cmake --build build --config Release --target Thudi_VST3` (fetches JUCE 8 automatically).
GitHub Actions builds, checks (headless tests, RHYNO measurements, pluginval), packages the setup wizard on every push,
and publishes builds from `main` as the `latest` release.
