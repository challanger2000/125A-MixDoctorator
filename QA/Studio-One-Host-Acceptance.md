# MixDoctorator — Studio One Host Acceptance Checklist

Status: manual host evidence. This checklist does not replace CI, Steinberg Validator or algorithm validation.

## Test setup

Record before testing:

- MixDoctorator commit:
- Studio One version:
- Windows version:
- Audio interface / driver:
- Sample rate:
- ASIO buffer:
- Test date:

Create one 8-bar loop with four tracks:

1. Drums — Sensor role Drums — Session A
2. Bass — Sensor role Bass — Session A
3. Electric Guitar 1 — Sensor role Electric Guitar — Session A
4. Electric Guitar 2 — Sensor role Electric Guitar — Session A

Insert one Brain on a bus or monitoring path and set Brain to Session A.

Do not change source gain or source audio during the acceptance pass.

## A. Connection and duplicate identity

- Start playback.
- Brain shows one Drums, one Bass and two Electric Guitar Sensors.
- Duplicate one guitar channel once.
- Confirm the duplicated instance is counted independently.
- Remove the duplicate again.
- Confirm the count returns without waiting for a stale timeout.

PASS / FAIL:
Notes:

## B. Four cycle wraps

Enable the 8-bar cycle and let it wrap at least four complete times.

Required:

- no clearing of the observed finding at a normal cycle wrap;
- no false new-session restart at the wrap;
- no sudden source-count loss caused only by the wrap;
- no audible interruption.

PASS / FAIL:
Notes:

## C. Stop and restart

While a meaningful diagnosis is visible:

1. Stop transport.
2. Wait at least 3 seconds.
3. Observe the Brain.
4. Restart playback.

Required while stopped:

- diagnostic levels, transient indicators, pair scores, finding and Coach recommendation remain frozen;
- connectivity and source counts are allowed to update;
- no stale moving meters should continue pretending that audio is being measured.

Required on restart:

- a fresh measurement starts;
- no stale response from the previous generation replaces the new measurement;
- no hang, click or state loss.

Repeat stop/restart at least 10 times in the host test.

PASS / FAIL:
Notes:

## D. Session migration

During playback:

1. Move Electric Guitar 2 from Session A to Session B.
2. Wait for the connection freshness period.
3. Confirm Session A no longer includes that Sensor.
4. Switch Brain to Session B and confirm the moved Sensor is visible there.
5. Move the guitar back to Session A.
6. Confirm Session A returns to two Electric Guitar Sensors.

Required:

- one live Sensor must never appear simultaneously in A and B;
- no old Session finding may appear as a new finding in the destination Session.

PASS / FAIL:
Notes:

## E. Save and reopen

With deliberately non-default role/session choices:

1. Save the Studio One project.
2. Close the project.
3. Reopen it.
4. Check every Sensor role and Session.
5. Check Brain Session.
6. Start playback.

Required:

- all role/session settings restored;
- duplicated channels remain distinct runtime instances;
- no stale runtime instance ID is restored from project state;
- analysis starts normally.

PASS / FAIL:
Notes:

## F. Editor lifecycle

During playback:

- open and close each Sensor editor repeatedly;
- open and close the Brain editor repeatedly;
- repeat while transport is stopped.

Use at least 20 open/close cycles for Brain and at least 10 for one Sensor.

Required:

- no crash or hang;
- no audio change caused by opening/closing the editor;
- no missing labels or controls;
- no lost Session/role state;
- no reset of analysis caused solely by editor open/close.

PASS / FAIL:
Notes:

## G. Sample-rate and buffer matrix

Repeat the core playback / stop / cycle checks at:

- 44.1 kHz
- 48 kHz
- 96 kHz

Use at least two practical ASIO buffer sizes at each rate where the driver permits it.

Record:

| Rate | Buffer | Playback | 4x cycle | Stop/restart | Editor open/close | Notes |
| --- | ---: | --- | --- | --- | --- | --- |
| 44.1 kHz |  |  |  |  |  |  |
| 44.1 kHz |  |  |  |  |  |  |
| 48 kHz |  |  |  |  |  |  |
| 48 kHz |  |  |  |  |  |  |
| 96 kHz |  |  |  |  |  |  |
| 96 kHz |  |  |  |  |  |  |

## H. Real host CPU / callback evidence

Measure separately with 1, 4 and 24 Sensor instances.

Record:

| Sensors | CPU mean | p95 | p99 | max | callback overruns / dropouts | Notes |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 |  |  |  |  |  |  |
| 4 |  |  |  |  |  |  |
| 24 |  |  |  |  |  |  |

The synthetic CI SensorLoadTests are supporting engineering evidence only. They are not a substitute for this paced realtime host measurement.

## Acceptance rule

Do not call the PoC host-qualified until every section above has a recorded PASS or a documented, understood exception.

Do not describe MASKING or ATTACK as calibrated probabilities. They remain experimental dimensionless indicators until real fixture/listening calibration is complete.
