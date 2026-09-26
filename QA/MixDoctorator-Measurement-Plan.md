# MixDoctorator — evidence and host-validation plan

Status: engineering plan, not release qualification. Applies to the Sensor/Brain PoC.
The numeric MASKING and ATTACK fields are experimental, dimensionless indicators.
They must not be described as calibrated probabilities or proof of audible masking.

## Baseline and provenance

- Baseline: Windows PoC commit `3f9fb0b418eda3787deda13d8369b48d055a32bd`, Actions run #41.
- Record host/version, operating system, sample rate, ASIO buffer size, plugin commit,
  exact Sensor roles and Session IDs, and whether the host transport is cycling.
- Keep source audio and gain unchanged between control and treatment.
- Save full session screenshots only as additional UI evidence; use exported measurements
  and repeatable audio fixtures for algorithmic acceptance.
- The user's initial 8-bar loop has one Drums, one Bass and two stereo E-Guitar
  channels. Both guitars must use Session A when Brain uses Session A.

## Host acceptance matrix (one deliberate DAW session)

1. Four channels / four Sensor instances on the same Session:
   verify 1 Drums, 1 Bass, 2 E-Guitar; stereo input is one Sensor.
2. Cycle an 8-bar section at least four times. Confirm wrap does not clear
   the observed finding or restart the session evidence.
3. Stop: all displayed diagnostic levels, transients, pair scores and findings
   remain frozen, while connectivity/count may update. Restart: new measurement
   is clearly distinguished from the previous session observation.
4. Switch one guitar to Session B: Session A count decreases to 1 after the
   connection freshness period; restore A and verify count returns to 2.
5. Save/reopen project: role and Session recall on every Sensor and Brain.
   Runtime instance IDs must stay distinct when a channel is duplicated.
6. Close/reopen editors repeatedly during playback and during stop. Confirm
   no audio change, hangs, missing labels or lost state.
7. Repeat at 44.1/48/96 kHz and at least two buffer sizes when practical.
8. Record CPU mean/p95/p99/max and callback overruns for 1, 4 and 24 Sensors,
   using paced realtime measurement; validator PASS alone is insufficient.

## Current automated evidence status

The following items are automated and green on Windows through Actions run #146:

- 44/44 CTest targets pass.
- Steinberg Validator: Sensor 537/537, Brain 537/537.
- Genuine independent-process shared-memory publication/read coherence.
- Simultaneous multi-process shared-memory initialization stress.
- IPC v10 abandoned writer-lock recovery after confirmed process termination.
- 24-slot capacity and immediate recovery after release.
- All 8 Sessions isolated; role/session state sanitization and legacy Sensor
  fallback covered.
- Four repeated transport cycle wraps do not reset analysis.
- 64 repeated stop/restart model cycles: stop freezes, restart resets.
- Stop publication ordering keeps connectivity/count updates ahead of the
  diagnostic freeze return.
- Analyzer safety covers silence, NaN/Inf, denormal and maximum finite input.
- Finite audio pass-through remains unchanged while pathological analysis
  arithmetic is bounded against overflow.
- Realtime process static guard rejects direct allocation, blocking primitives,
  filesystem/console I/O and direct slow IPC access in process().
- Synthetic Sensor analysis timing covers 1/4/24 Sensors at 48 kHz / 256
  samples and 4 Sensors at 44.1/48/96 kHz with 128/256/512 samples. Current
  CI reports zero measured deadline overruns in these synthetic tests.

The following remain **host/manual or fixture-dependent** and must not be
claimed from CI alone:

- Studio One project save/reopen with all Sensor roles/Sessions and Brain
  Session restored.
- Repeated real editor open/close while playing/stopped.
- Real Studio One 4-Sensor Session A/B migration and freshness timing.
- Paced realtime host CPU/callback-overrun measurement for 1/4/24 Sensors.
- Multi-host portability beyond Steinberg Validator.
- Real immutable drums/bass/guitar/full-mix audio fixtures, blinded
  level-matched listening, and false-positive/false-negative calibration.
- Final 125A release QA / Plugin Tester qualification.

## Algorithm validation before calibrating thresholds

Create cleared, immutable stereo files at original levels for drums, bass,
left/right rhythm guitar, and a dense full mix. Record provenance, rate and
bit depth. Add negative controls (disjoint bands, one source silent, separated
attacks, substantially quieter competing source) and positive controls
(known intentional same-band competition and deliberately coincident attacks).
Use identical gain structure and timing for before/after comparisons.

For each file or control:
- Log raw band energies, spectral overlap, relative source level, envelope
  and transient indicator separately from the aggregate diagnostic.
- Log time windows, sample-position coherence and fraction of valid frames.
- Repeat with two guitars separate and merged, and document how aggregation
  changes results. Do not treat role-level aggregation as per-guitar diagnosis.
- Compare the assistant recommendation to blinded, level-matched listening
  observations. An elevated algorithmic indicator is not itself a mix defect.
- Define false-positive and false-negative acceptance criteria from these
  observations before changing numeric thresholds.
- Document estimated/empirical constants and keep revisions traceable.

## Measurement and code-review gates

- Run pure model tests locally/static-check first, then one intentional CI build.
- Verify 32/64-bit processing, pass-through identity, silence and finite
  analysis behavior on NaN/Inf, denormal and extreme but valid inputs.
- Stress sensor slot ownership, short disconnect/reconnect, duplicate channels,
  multi-session isolation and concurrent independent host processes.
- Crash-recovery gate: deliberately terminate a Sensor process while it owns a
  shared-memory writer slot and verify that no writerLock can remain permanently
  orphaned.
  - AUTOMATED GREEN as of Windows Actions run #146: IPC v10 writer locks carry
    the owning Windows process ID; a live owner is not stolen, while a stale
    lock from a confirmed terminated process can be recovered. The dedicated
    IPCConcurrencyTests report passes in CI.
- Audit Windows shared-memory access and audio-thread timing against the
  realtime contract. Current IPC is explicitly PoC and not release-qualified.
- Review VST3 ProcessContext/cycle flags and state/lifecycle behavior in
  multiple hosts before treating host-specific behavior as portable.
- Require Steinberg validator and 125A release QA, not just successful compile.
