from pathlib import Path

files = [
    Path("src/BrainController.cpp"),
    Path("resource/Brain.uidesc"),
]

forbidden = [
    "Masking-Hinweis",
    "Masking-Wahrscheinlichkeiten",
    "Attack-Konkurrenz",
    "bei Attack:",
    "in den Attacks",
    "klarere Attacks",
    "Attack- und harmonische",
    "Schlagzeug-Attack",
    "bei Attack/Praesenz",
    "sanftes Ducking",
    "getrennte Parts",
    "Punch",
    "Hoehen oder Air",
    "Timing oder Ducking",
    "AIR 10k+",
]

violations = []

for path in files:
    text = path.read_text(encoding="utf-8")
    for phrase in forbidden:
        if phrase in text:
            violations.append(f"{path}: visible English phrase remains: {phrase}")

if violations:
    raise SystemExit("\n".join(violations))

print("German visible Coach wording check passed")
