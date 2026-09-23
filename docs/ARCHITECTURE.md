# MixDoctorator PoC Architecture

## Scope

The first proof of concept contains two normal VST3 plug-ins:

- **125A MixDoctorator Sensor**
- **125A MixDoctorator Brain**

Three Sensor instances are used initially:

- Drums
- Bass
- Electric Guitar

The system analyses and reports only. It does not modify audio.

## Data flow

```text
Audio track/bus
    |
    v
Sensor VST3
    |- transparent audio pass-through
    |- local realtime-safe measurements
    |- role: Drums / Bass / Electric Guitar
    |
    v
lock-free handoff
    |
    v
non-realtime IPC publisher
    |
    v
OS shared-memory session
    |
    v
Brain VST3
    |- discovers live sensors
    |- reads timestamped analysis frames
    |- keeps sources separated
    |- compares source relationships
    |- presents findings
```

## Important architectural constraint

A process-local singleton is **not sufficient**.

DAWs may host different plug-in instances in different processes. Therefore the existing process-local registry pattern used elsewhere in 125A projects can be useful for unit-test ideas, but it cannot be the transport contract for MixDoctorator.

The PoC transport must survive:

```text
Sensor A = process 1
Sensor B = process 2
Sensor C = process 3
Brain    = process 4
```

For the initial Windows build, named shared memory is the preferred transport candidate.

## Realtime rule

The audio thread must never:

- wait for the Brain
- acquire a blocking inter-process mutex
- perform file I/O
- allocate memory as part of normal processing
- call slow OS IPC routines directly

The Sensor audio callback only performs bounded analysis and publishes into a lock-free/local mailbox. A non-realtime worker publishes compact analysis frames to IPC.

## Sensor identity

Each Sensor owns:

- persistent instance UUID
- selected musical role
- optional user label
- heartbeat / generation
- timestamp / host sample position when available

Initial role enum:

```text
Unknown
Drums
Bass
ElectricGuitar
```

The role is manually selectable in the Sensor UI. Host track-name detection is optional convenience only.

## Initial analysis frame

Keep V0 deliberately small:

```text
instanceId
role
sequence
hostSamplePosition
rmsDb
peakDb
lowEnergy
lowMidEnergy
presenceEnergy
transientStrength
activity
```

No full-rate audio is transported to the Brain.

## First success criterion

With Studio One/Fender Studio playing:

```text
Drums           CONNECTED
Bass            CONNECTED
Electric Guitar CONNECTED
```

The Brain must show independent, changing measurement values for all three sources.

The test must remain stable when:

- transport starts/stops
- plug-in windows are closed
- Sensor order changes
- a Sensor is removed/re-added
- the project is saved/reloaded

## Second milestone

Only after transport is proven:

- synchronized comparison windows
- pairwise spectral-overlap scoring
- activity-aware conflict detection
- simple ranked findings

Example:

```text
1. Bass <-> Electric Guitar
   repeated low-mid overlap

2. Drums <-> Bass
   moderate low-end competition

3. Drums <-> Electric Guitar
   no urgent issue
```

## Explicitly out of scope for V0

- automatic correction
- remote DSP control
- AI/ML claims
- automatic instrument recognition
- Mix FX implementation
- sidechain routing
- full mastering analysis
- product-ready GUI
