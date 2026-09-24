from pathlib import Path
import re
import sys

CASES = [
    (
        Path("src/BrainIDs.h"),
        Path("src/BrainController.cpp"),
        Path("src/BrainProcessor.cpp"),
        "Brain",
    ),
    (
        Path("src/SensorIDs.h"),
        Path("src/SensorController.cpp"),
        Path("src/SensorProcessor.cpp"),
        "Sensor",
    ),
]


def enum_names(path: Path):
    text = path.read_text(encoding="utf-8")
    m = re.search(
        r"enum\s+ParamID\s*:[^{]+\{(?P<body>.*?)\};",
        text,
        re.S,
    )
    if not m:
        raise RuntimeError(f"{path}: ParamID enum not found")
    return set(re.findall(r"\b(k[A-Za-z0-9_]+)\b", m.group("body")))


def registered_names(path: Path):
    text = path.read_text(encoding="utf-8")
    names = set()

    # Standard addParameter(..., flags, kParam): the ParamID is the final
    # argument. Do not collect VST3 flags such as kIsHidden/kIsReadOnly.
    for m in re.finditer(
        r"parameters\.addParameter\s*\((?P<body>.*?)\);",
        text,
        re.S,
    ):
        p = re.search(
            r",\s*(k[A-Za-z0-9_]+)\s*$",
            m.group("body"),
            re.S,
        )
        if p:
            names.add(p.group(1))

    # StringListParameter(name, kParam, ...): ParamID is the second argument.
    for m in re.finditer(
        r"new\s+StringListParameter\s*\(\s*"
        r"STR16\([^\)]*\)\s*,\s*"
        r"(?P<param>k[A-Za-z0-9_]+)",
        text,
        re.S,
    ):
        names.add(m.group("param"))

    return names


def published_names(path: Path):
    text = path.read_text(encoding="utf-8")
    names = set()
    for m in re.finditer(
        r"publishParam\s*\((?P<body>.*?)\);",
        text,
        re.S,
    ):
        body = m.group("body")
        if not re.match(r"\s*data\s*,", body):
            continue
        p = re.match(
            r"\s*data\s*,\s*(k[A-Za-z0-9_]+)\s*,",
            body,
            re.S,
        )
        if p:
            names.add(p.group(1))
    return names


errors = []

for ids_path, controller_path, processor_path, label in CASES:
    ids = enum_names(ids_path)
    registered = registered_names(controller_path)
    published = published_names(processor_path)

    missing_registration = sorted(ids - registered)
    unknown_registration = sorted(registered - ids)
    unregistered_publish = sorted(published - registered)
    unknown_publish = sorted(published - ids)

    if missing_registration:
        errors.append(
            f"{label}: enum IDs not registered in controller: "
            + ", ".join(missing_registration)
        )
    if unknown_registration:
        errors.append(
            f"{label}: controller references unknown IDs: "
            + ", ".join(unknown_registration)
        )
    if unregistered_publish:
        errors.append(
            f"{label}: processor publishes unregistered IDs: "
            + ", ".join(unregistered_publish)
        )
    if unknown_publish:
        errors.append(
            f"{label}: processor publishes unknown IDs: "
            + ", ".join(unknown_publish)
        )

if errors:
    print("Parameter registration consistency FAILED")
    for error in errors:
        print(error)
    sys.exit(1)

print("Parameter registration consistency passed")
