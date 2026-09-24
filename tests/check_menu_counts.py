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

sensor_roles = len(re.findall(r'role->appendString\s*\(', sensor_role_block))
sensor_sessions = len(re.findall(r'session->appendString\s*\(', sensor_session_block))
brain_sessions = len(re.findall(r'session->appendString\s*\(', brain_session_block))

errors = []

if sensor_roles != role_count:
    errors.append(
        f"Sensor role menu has {sensor_roles} entries, IPC kRoleCount={role_count}"
    )

if sensor_sessions != session_count:
    errors.append(
        f"Sensor session menu has {sensor_sessions} entries, IPC kSessionCount={session_count}"
    )

if brain_sessions != session_count:
    errors.append(
        f"Brain session menu has {brain_sessions} entries, IPC kSessionCount={session_count}"
    )

if errors:
    print("Menu count consistency FAILED")
    for error in errors:
        print(error)
    sys.exit(1)

print(
    "Menu count consistency passed: "
    f"{role_count} roles, {session_count} sessions"
)
