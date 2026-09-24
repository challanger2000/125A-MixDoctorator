from pathlib import Path
import re
import sys

LIMIT = 118
FILES = [
    Path("src/BrainController.cpp"),
    Path("src/SensorController.cpp"),
]

literal = re.compile(r'"([^"\\]*(?:\\.[^"\\]*)*)"')
failures = []

for path in FILES:
    text = path.read_text(encoding="utf-8")
    for line_no, line in enumerate(text.splitlines(), 1):
        for match in literal.finditer(line):
            value = match.group(1)
            # Include paths and formatting strings are harmless, but are
            # naturally short. Flag every long literal so UI text cannot
            # silently overflow Steinberg String128 presentation buffers.
            if len(value) > LIMIT:
                failures.append(
                    f"{path}:{line_no}: {len(value)} chars: {value}"
                )

if failures:
    print("String128 safety check FAILED")
    for failure in failures:
        print(failure)
    sys.exit(1)

print(f"String128 safety check passed (limit {LIMIT})")
