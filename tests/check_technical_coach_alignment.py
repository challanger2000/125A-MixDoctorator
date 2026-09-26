from pathlib import Path
import sys

uidesc = Path("resource/Brain.uidesc").read_text(encoding="utf-8")

required = [
    'title="WICHTIGSTER VERDECKUNGS-HINWEIS — ALLE ROLLEN"',
    'control-tag="CoachPair"',
    'control-tag="CoachMasking"',
    'control-tag="CoachBand"',
    'control-tag="CoachDominance"',
    'control-tag="CoachConfidence"',
]

errors = [item for item in required if item not in uidesc]

headline_start = uidesc.find('title="WICHTIGSTER VERDECKUNGS-HINWEIS — ALLE ROLLEN"')
attack_start = uidesc.find('title="AKTUELLER ANSCHLAG-HINWEIS"')

if headline_start < 0 or attack_start < 0 or attack_start <= headline_start:
    errors.append("technical global-diagnosis section boundaries not found")
else:
    section = uidesc[headline_start:attack_start]
    for forbidden in (
        'control-tag="TopPair"',
        'control-tag="SessionPair"',
        'control-tag="TopScore"',
        'control-tag="SessionScore"',
    ):
        if forbidden in section:
            errors.append(
                f"legacy three-pair binding remains in global diagnosis: {forbidden}"
            )

if errors:
    print("Technical Coach alignment FAILED")
    for error in errors:
        print(error)
    sys.exit(1)

print("Technical Coach alignment passed")
