from pathlib import Path
import re
import sys

ipc = Path("src/MixDoctoratorIPC.h").read_text(encoding="utf-8")
sensor = Path("src/SensorController.cpp").read_text(encoding="utf-8")
brain = Path("src/BrainController.cpp").read_text(encoding="utf-8")

role_match = re.search(r"constexpr\s+int\s+kRoleCount\s*=\s*(\d+)\s*;", ipc)
session_match = re.search(r"constexpr\s+int\s+kSessionCount\s*=\s*(\d+)\s*;", ipc)

if not role_match or not session_match:
    print("Menu count consistency FAILED: IPC counts not found")
    sys.exit(1)

role_count = int(role_match.group(1))
session_count = int(session_match.group(1))

role_enum_match = re.search(
    r"enum\s+class\s+Role\s*:\s*std::uint32_t\s*\{(?P<body>.*?)\};",
    ipc,
    re.S,
)

if not role_enum_match:
    print("Menu count consistency FAILED: Role enum not found")
    sys.exit(1)

role_enum = [
    name
    for name in re.findall(
        r"\b([A-Za-z][A-Za-z0-9_]*)\s*=\s*\d+",
        role_enum_match.group("body"),
    )
    if name != "Unknown"
]

expected_role_labels = {
    "Drums": "Schlagzeug",
    "Bass": "Bass",
    "ElectricGuitar": "E-Gitarre",
    "Kick": "Kick",
    "Snare": "Snare",
    "Toms": "Toms",
    "Cymbals": "Becken / Hi-Hat",
    "Percussion": "Perkussion",
    "AcousticGuitar": "Akustikgitarre",
    "LeadVocal": "Hauptgesang",
    "BackingVocal": "Hintergrundgesang",
    "PianoKeys": "Piano / Tasten",
    "Synth": "Synthesizer",
    "Pad": "Flaeche",
}


def block_between(text, start_token, end_token):
    start = text.find(start_token)
    if start < 0:
        return ""
    end = text.find(end_token, start)
    if end < 0:
        return ""
    return text[start:end]


sensor_role_block = block_between(
    sensor,
    'auto* role=new StringListParameter',
    'parameters.addParameter(role);',
)

sensor_session_block = block_between(
    sensor,
    'auto* session=new StringListParameter',
    'parameters.addParameter(session);',
)

brain_session_block = block_between(
    brain,
    'auto* session=new StringListParameter',
    'parameters.addParameter(session);',
)

sensor_role_labels = re.findall(
    r'role->appendString\s*\(\s*STR16\("([^"]+)"\)\s*\)',
    sensor_role_block,
)
sensor_session_labels = re.findall(
    r'session->appendString\s*\(\s*STR16\("([^"]+)"\)\s*\)',
    sensor_session_block,
)
brain_session_labels = re.findall(
    r'session->appendString\s*\(\s*STR16\("([^"]+)"\)\s*\)',
    brain_session_block,
)

sensor_roles = len(sensor_role_labels)
sensor_sessions = len(sensor_session_labels)
brain_sessions = len(brain_session_labels)

errors = []

if sensor_roles != role_count:
    errors.append(
        f"Sensor role menu has {sensor_roles} entries, IPC kRoleCount={role_count}"
    )

if role_enum != list(expected_role_labels):
    errors.append(
        "Role enum order differs from the UI label contract: "
        + ", ".join(role_enum)
    )

expected_sensor_role_labels = [
    expected_role_labels[name]
    for name in role_enum
    if name in expected_role_labels
]

if sensor_role_labels != expected_sensor_role_labels:
    errors.append(
        "Sensor role menu order/labels do not match Role enum: "
        + " | ".join(sensor_role_labels)
    )

expected_sessions = [
    chr(ord("A") + i)
    for i in range(session_count)
]

if sensor_sessions != session_count:
    errors.append(
        f"Sensor session menu has {sensor_sessions} entries, IPC kSessionCount={session_count}"
    )

if sensor_session_labels != expected_sessions:
    errors.append(
        "Sensor session labels/order mismatch: "
        + " | ".join(sensor_session_labels)
    )

if brain_sessions != session_count:
    errors.append(
        f"Brain session menu has {brain_sessions} entries, IPC kSessionCount={session_count}"
    )

if brain_session_labels != expected_sessions:
    errors.append(
        "Brain session labels/order mismatch: "
        + " | ".join(brain_session_labels)
    )

if errors:
    print("Menu count consistency FAILED")
    for error in errors:
        print(error)
    sys.exit(1)

print(
    "Menu order consistency passed: "
    f"{role_count} roles, {session_count} sessions"
)
