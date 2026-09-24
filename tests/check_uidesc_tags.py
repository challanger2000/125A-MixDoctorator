from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

CASES = [
    (Path("src/BrainIDs.h"), Path("resource/Brain.uidesc")),
    (Path("src/SensorIDs.h"), Path("resource/Sensor.uidesc")),
]


def parse_param_enum(path: Path):
    text = path.read_text(encoding="utf-8")
    match = re.search(
        r"enum\s+ParamID\s*:[^{]+\{(?P<body>.*?)\};",
        text,
        re.S,
    )
    if not match:
        raise RuntimeError(f"{path}: ParamID enum not found")

    values = {}
    current = -1

    for raw in match.group("body").split(","):
        item = re.sub(r"//.*", "", raw).strip()
        if not item:
            continue

        m = re.match(
            r"(?P<name>k[A-Za-z0-9_]+)(?:\s*=\s*(?P<value>0x[0-9A-Fa-f]+|\d+))?$",
            item,
        )
        if not m:
            continue

        name = m.group("name")
        explicit = m.group("value")

        if explicit is not None:
            current = int(explicit, 0)
        else:
            current += 1

        values[name] = current

    return values


def validate(ids_path: Path, ui_path: Path):
    errors = []
    params = parse_param_enum(ids_path)

    root = ET.parse(ui_path).getroot()
    tags = root.find("control-tags")

    if tags is None:
        return [f"{ui_path}: <control-tags> section missing"]

    by_name = {}
    by_number = {}

    for node in tags.findall("control-tag"):
        name = node.attrib.get("name")
        raw_tag = node.attrib.get("tag")

        if not name or raw_tag is None:
            errors.append(f"{ui_path}: malformed <control-tag>")
            continue

        try:
            number = int(raw_tag, 0)
        except ValueError:
            errors.append(
                f"{ui_path}: control-tag {name!r} has invalid tag {raw_tag!r}"
            )
            continue

        if name in by_name:
            errors.append(
                f"{ui_path}: duplicate control-tag name {name!r}"
            )
        by_name[name] = number

        if number in by_number:
            errors.append(
                f"{ui_path}: duplicate numeric tag {number}: "
                f"{by_number[number]!r} and {name!r}"
            )
        by_number[number] = name

        enum_name = f"k{name}"
        if enum_name not in params:
            errors.append(
                f"{ui_path}: {name!r} has no matching {enum_name} in {ids_path}"
            )
        elif params[enum_name] != number:
            errors.append(
                f"{ui_path}: {name!r} tag={number}, "
                f"but {enum_name}={params[enum_name]} in {ids_path}"
            )

    used = set()

    for node in root.iter():
        control = node.attrib.get("control-tag")
        if control:
            used.add(control)
            if control not in by_name:
                errors.append(
                    f"{ui_path}: view references undefined control-tag {control!r}"
                )

    # A defined UI tag that is never used is usually stale wiring.
    for name in sorted(set(by_name) - used):
        errors.append(
            f"{ui_path}: defined control-tag {name!r} is never used by a view"
        )

    return errors


all_errors = []

for ids_path, ui_path in CASES:
    all_errors.extend(validate(ids_path, ui_path))

if all_errors:
    print("UIDesc parameter/tag consistency FAILED")
    for error in all_errors:
        print(error)
    sys.exit(1)

print("UIDesc parameter/tag consistency passed")
