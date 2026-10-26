# Thudi (துடி) — drum synthesizer by R4N

A 16-pad drum **synthesizer** (no samples) as a VST3 instrument. Every pad is a full synth voice
(tone + noise + metal + body/resonator layers) with its own character chain
(oversampled drive, crush above a crossover, chorus/width with mono bass, tape, cuts),
built around the sound of RHYNO.

* `src/dsp` – engine in plain C++ (no JUCE): `Voice`, `PadChain`, `Engine`, parameter table, models, kit JSON
* `src/plugin` – JUCE plugin: processor (1 088 automatable parameters, 16 stereo outs), presets, GUI
* `art/blender` – procedural 3D model of the panel, rendered with Cycles in CI → `assets/ui`
* `presets/factory` – factory kits (`RHYNO`, `808 CLASSIC`, `GRIT LAB`) and 48 pad presets
* `tools` – offline renderer, headless plugin checks/screenshots, RHYNO metrics and fitting, Tamil logo shaping

Build: `cmake -S . -B build && cmake --build build --config Release --target Thudi_VST3`
(GitHub Actions builds the Windows VST3 on every push; see `INSTALL.md`).

Fonts: Kavivanar, VT323, Barlow Condensed — SIL Open Font License (see `assets/fonts`).
