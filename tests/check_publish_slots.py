from pathlib import Path
import re
import sys

cpp = Path("src/BrainProcessor.cpp").read_text(encoding="utf-8")
hdr = Path("src/BrainProcessor.h").read_text(encoding="utf-8")

size_match = re.search(r"double\s+last_\[(\d+)\]", hdr)
if not size_match:
    print("Publication slot check FAILED: last_ array size not found")
    sys.exit(1)

slot_count = int(size_match.group(1))

calls = re.findall(
    r"publishParam\s*\((?P<body>.*?)\);",
    cpp,
    re.S,
)

by_slot = {}
by_param = {}
errors = []
checked = 0

for body in calls:
    # Ignore the function signature/definition if it is ever matched by
    # future formatting changes. Real calls always start with data.
    if not re.match(r"\s*data\s*,", body):
        continue

    param_match = re.match(
        r"\s*data\s*,\s*(k[A-Za-z0-9_]+)\s*,",
        body,
        re.S,
    )
    slot_match = re.search(r",\s*(\d+)\s*$", body, re.S)

    if not param_match or not slot_match:
        errors.append(
            "Could not parse publishParam call: "
            + " ".join(body.split())[:180]
        )
        continue

    param = param_match.group(1)
    slot = int(slot_match.group(1))
    checked += 1

    if slot < 0 or slot >= slot_count:
        errors.append(
            f"{param}: slot {slot} outside last_[{slot_count}]"
        )

    previous_param = by_slot.get(slot)
    if previous_param is not None and previous_param != param:
        errors.append(
            f"slot {slot} shared by {previous_param} and {param}"
        )
    by_slot[slot] = param

    previous_slot = by_param.get(param)
    if previous_slot is not None and previous_slot != slot:
        errors.append(
            f"{param} published through multiple slots: "
            f"{previous_slot} and {slot}"
        )
    by_param[param] = slot

if checked == 0:
    errors.append("No publishParam calls were checked")

if errors:
    print("Publication slot consistency FAILED")
    for error in errors:
        print(error)
    sys.exit(1)

print(
    "Publication slot consistency passed: "
    f"{checked} calls, {len(by_slot)} slots, last_[{slot_count}]"
)
