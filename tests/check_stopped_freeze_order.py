from pathlib import Path
import sys

source = Path("src/BrainProcessor.cpp").read_text(encoding="utf-8")

sensor_count = source.find("kAllSensorCount")
role_count = source.find("kAllRoleCount")
stop_return = source.find("if(hasProcessContext && !playing)")
drums_level = source.find("kDrumsLevel", stop_return)
top_pair = source.find("kTopPair", stop_return)

errors = []

if sensor_count < 0 or role_count < 0 or stop_return < 0:
    errors.append("Required stop/freeze markers not found")
else:
    if sensor_count > stop_return:
        errors.append("Sensor count publication must occur before stopped return")
    if role_count > stop_return:
        errors.append("Role count publication must occur before stopped return")

if drums_level < 0 or top_pair < 0:
    errors.append("Frozen diagnostic publications not found after stopped return")

if errors:
    print("Stopped diagnostic freeze consistency FAILED")
    for error in errors:
        print(f"  {error}")
    sys.exit(1)

print(
    "Stopped diagnostic freeze consistency passed: "
    "connectivity/counts update before stop return; diagnosis stays frozen"
)
