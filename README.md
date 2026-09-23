# 125A MixDoctorator

Experimental multi-instance mix analysis system with track sensors and a central mix brain.

## Concept

125A MixDoctorator is a proof-of-concept VST3 system for analysing relationships between multiple mix sources without modifying the audio.

Lightweight **Sensor** instances are placed on individual tracks or buses. A central **Brain** instance collects their analysis data, compares the sources and reports potential mix problems in musician-friendly language.

## First proof of concept

Initial source roles:

- Drums
- Bass
- Electric Guitar

Initial architecture:

```text
Drums Bus  -> MixDoctorator Sensor [Drums]  --\
Bass       -> MixDoctorator Sensor [Bass]    ---> MixDoctorator Brain -> analysis/report
E-Guitar   -> MixDoctorator Sensor [Guitar] --/
```

## V0 goals

1. Build two VST3 plug-ins:
   - **125A MixDoctorator Sensor**
   - **125A MixDoctorator Brain**
2. Sensor role is selectable directly in the Sensor UI.
3. Sensor audio is passed through unchanged.
4. Each Sensor computes lightweight local analysis data.
5. Sensor instances register with the Brain through process-safe IPC.
6. The Brain keeps sources separate and displays connection/status data.
7. No corrective DSP in V0.
8. No automatic mixing in V0.

## First technical milestone

Prove that three independent VST3 Sensor instances can reliably send timestamped analysis data to one Brain instance while the DAW is playing.

The first successful test is simply:

```text
Drums   CONNECTED
Bass    CONNECTED
Guitar  CONNECTED
```

with independent live analysis values for all three sources.

## Design principles

- Audio thread must never wait on the Brain.
- No blocking locks, file I/O or IPC calls in the realtime audio path.
- Sensor-to-Brain communication must remain valid if plug-ins are hosted in separate processes.
- Manual role selection is the reliable baseline; host track-name detection may be added only as an optional convenience.
- V1 analyses and advises only. Any automatic correction is explicitly deferred to a later version.

## Status

Early research / proof of concept.
