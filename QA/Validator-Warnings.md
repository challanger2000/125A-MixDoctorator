# MixDoctorator — Validator warnings and accepted exceptions

Status: PoC engineering record. This file does not turn an accepted warning into release qualification.

## Steinberg Validator: no bypass parameter

Current Sensor and Brain validator runs pass all validator tests but report:

> Warning: No bypass parameter found. Is this intended ?

This warning is currently **acknowledged, not fixed blindly**.

Reason:

- Sensor and Brain are analysis plug-ins and already pass finite audio through transparently.
- Adding a VST3 bypass parameter is not only a validator-cleanup change; it defines product behaviour.
- Sensor bypass must explicitly decide whether analysis and IPC publication continue, freeze, or disconnect.
- Brain bypass must explicitly decide whether existing diagnostics remain visible, freeze, reset, or disconnect from Sensor data.
- Those choices affect automation, state recall, Session behaviour and host expectations.

Therefore no bypass parameter is added during the current PoC QA pass merely to remove the warning.

Before release qualification, define and test bypass semantics in Studio One and at least one additional VST3 host, then either:

1. implement a standard automatable VST3 bypass parameter with documented Sensor/Brain behaviour, or
2. retain the intentional no-bypass design and document the host-facing consequence.

## Other current build warnings

The following warnings observed in successful Windows CI are not from MixDoctorator product source:

- VSTGUI C4267 conversion warning inside Steinberg/VSTGUI source;
- MSVC D9025 on test targets because Release tests intentionally undefine NDEBUG so assert-based QA remains active;
- GitHub Action / Node deprecation warnings emitted by third-party actions.

These must not be reported as MixDoctorator product-code warnings.
