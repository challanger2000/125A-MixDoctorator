from pathlib import Path
import re
import sys

cmake = Path("CMakeLists.txt").read_text(encoding="utf-8")
workflow = Path(".github/workflows/build-windows.yml").read_text(
    encoding="utf-8"
)

expected = set(
    re.findall(
        r"add_executable\((\w+Tests)\s+tests/[^\)]+\)",
        cmake,
        re.S,
    )
)

build_match = re.search(
    r"cmake --build build --config Release --target (?P<body>[^\n]+)",
    workflow,
)

if not build_match:
    print("Windows test target consistency FAILED")
    print("Build target line not found")
    sys.exit(1)

listed = set(
    re.findall(
        r"\b(\w+Tests)\b",
        build_match.group("body"),
    )
)

missing = sorted(expected - listed)
stale = sorted(listed - expected)

if missing or stale:
    print("Windows test target consistency FAILED")

    if missing:
        print("Missing from Windows build target list:")
        for name in missing:
            print(f"  {name}")

    if stale:
        print("Stale Windows test targets:")
        for name in stale:
            print(f"  {name}")

    sys.exit(1)

print(
    "Windows test target consistency passed "
    f"({len(expected)} test targets)"
)
