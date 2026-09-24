from pathlib import Path
import re
import sys

cmake = Path("CMakeLists.txt").read_text(encoding="utf-8")

targets = re.findall(
    r"add_executable\((\w+Tests)\s+tests/[^\)]+\)",
    cmake,
    re.S,
)

m = re.search(
    r"set\(MIXDOCTORATOR_TEST_TARGETS(?P<body>.*?)\)",
    cmake,
    re.S,
)

if not m:
    print("Release assert target list missing")
    sys.exit(1)

listed = set(re.findall(r"\b(\w+Tests)\b", m.group("body")))
expected = set(targets)

missing = sorted(expected - listed)
stale = sorted(listed - expected)

if missing or stale:
    print("Release assert target consistency FAILED")
    if missing:
        print("Missing from MIXDOCTORATOR_TEST_TARGETS:")
        for name in missing:
            print(f"  {name}")
    if stale:
        print("Stale entries in MIXDOCTORATOR_TEST_TARGETS:")
        for name in stale:
            print(f"  {name}")
    sys.exit(1)

print(
    "Release assert target consistency passed "
    f"({len(expected)} test targets)"
)
