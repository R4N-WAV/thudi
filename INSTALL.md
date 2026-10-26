# Installere Thudi (துடி) i FL Studio — steg for steg

1. **Pakk ut** `Thudi-VST3-Windows.zip`. Du får en mappe som heter `Thudi.vst3`.
2. **Kopier** hele `Thudi.vst3`-mappen til
   `C:\Program Files\Common Files\VST3\`
   (Windows spør om administrator-tillatelse – trykk **Fortsett**.)
3. **Åpne FL Studio** → `Options` → `Manage plugins` → trykk **Find installed plugins** (eller *Start scan*).
4. Finn **Thudi** (produsent **R4N**) i listen og huk av ★ (favoritt) så den dukker opp i plugin-menyen.
5. I Channel Rack: **+** → **Thudi**. Pluginen åpner med RHYNO-kitet lastet.

## Bruk
- **Klikk på en pad** for å spille den (høyere oppe på paden = hardere slag) og velge den for redigering.
- **Høyreklikk på en pad**: Edit sound, Rename, Copy/Paste, Swap, Copy to, Model, Init as, Load/Save pad preset,
  Randomize, Choke group, Output, Link, Play mode, spill kromatisk fra keyboard, MIDI learn.
- **De 8 knottene** under LCD-en styrer siden som vises. `SOFT KEY 1/2` blar mellom sidene.
- **SOFT KEY 3 (BROWSE)**: bla i kits og pad-presets. Hjulet (DATA ENTRY) eller musehjulet blar, **ENTER** laster.
- **SOFT KEY 4 (SAVE)**: lagre kit eller pad-preset. Egne presets ligger i `Dokumenter\R4N\Thudi\`.
- Dobbelklikk på en knott = standardverdi.

## MIDI
Pad 1–16 = C1–D#2 (MIDI-note 36–51), nederste rad er pad 1–4 (som på en MPC).
RHYNO-kitet: pad 1 (KICK) trigger 808-en på pad 2 (Link). 808-en er i **Gate**-modus: lengden på noten i
piano roll bestemmer hvor lenge 808-en varer (bounce). Toner utenfor pad-notene spiller 808-en kromatisk.

## Flere utganger (valgfritt)
Sett `Output` på en pad (høyreklikk → Output). I FL: Thudi-vinduets tannhjul → *Processing* →
aktiver flere utganger, og rut dem til mixerkanaler.
