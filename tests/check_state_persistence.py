from pathlib import Path
import sys

sensor = Path("src/SensorProcessor.cpp").read_text(encoding="utf-8")
brain = Path("src/BrainProcessor.cpp").read_text(encoding="utf-8")

errors = []

sensor_get = sensor.find("Processor::getState")
sensor_set = sensor.find("Processor::setState")
brain_get = brain.find("Processor::getState")
brain_set = brain.find("Processor::setState")

if min(sensor_get, sensor_set, brain_get, brain_set) < 0:
    errors.append("Missing processor state method")
else:
    sensor_get_body = sensor[sensor_get:]
    sensor_set_body = sensor[sensor_set:]
    brain_get_body = brain[brain_get:]
    brain_set_body = brain[brain_set:]

    if sensor_get_body.count("writeInt32") < 2:
        errors.append("Sensor getState must persist role and session")

    if "legacySensorSessionFallback" not in sensor_set_body:
        errors.append("Sensor setState must preserve legacy missing-session fallback")

    if "sanitizeRoleState" not in sensor_set_body:
        errors.append("Sensor setState must sanitize role")

    if brain_get_body.count("writeInt32") < 1:
        errors.append("Brain getState must persist session")

    if "sanitizeSessionState" not in brain_set_body:
        errors.append("Brain setState must sanitize restored session")

if errors:
    print("Processor state persistence consistency FAILED")
    for error in errors:
        print(f"  {error}")
    sys.exit(1)

print(
    "Processor state persistence consistency passed: "
    "Sensor role/session + legacy fallback; Brain session"
)
