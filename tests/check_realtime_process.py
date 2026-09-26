from pathlib import Path
import re
import sys

FILES = [
    Path("src/BrainProcessor.cpp"),
    Path("src/SensorProcessor.cpp"),
]

BANNED_COMMON = [
    "sleep_for",
    "Sleep(",
    "std::mutex",
    "lock_guard",
    "unique_lock",
    "std::thread",
    "random_device",
    "std::vector",
    "std::string",
    "make_unique",
    "make_shared",
    "malloc(",
    "calloc(",
    "realloc(",
    "free(",
    "std::filesystem",
    "ofstream",
    "ifstream",
    "fopen(",
    "printf(",
    "std::cout",
    "std::cerr",
    "CreateFileMapping",
    "MapViewOfFile",
    "ipc_.readSlot",
    "ipc_.publish",
]

BANNED_BRAIN_ONLY = [
    "measurePair(",
    "evaluateCoachMasking(",
    "evaluatePairWithNeighborSpread(",
]


def extract_process_region(text: str):
    start = text.find("Processor::process(")
    if start < 0:
        return None

    end = text.find("Processor::setState(", start)
    if end < 0:
        end = len(text)

    return text[start:end]


errors = []

for path in FILES:
    text = path.read_text(encoding="utf-8")
    region = extract_process_region(text)

    if region is None:
        errors.append(f"{path}: Processor::process not found")
        continue

    # Strip // comments to avoid false positives from documentation.
    region = re.sub(r"//.*", "", region)

    banned = list(BANNED_COMMON)
    if path.name == "BrainProcessor.cpp":
        banned.extend(BANNED_BRAIN_ONLY)

    for token in banned:
        if token in region:
            errors.append(
                f"{path}: forbidden realtime token in process(): {token}"
            )

    # Catch obvious direct heap allocation/deallocation syntax. Word
    # boundaries avoid matching identifiers such as newest.
    if re.search(r"\bnew\s+[A-Za-z_:]", region):
        errors.append(f"{path}: direct new allocation in process()")
    if re.search(r"\bdelete\s+", region):
        errors.append(f"{path}: direct delete in process()")

if errors:
    print("Realtime process guard FAILED")
    for error in errors:
        print(error)
    sys.exit(1)

print("Realtime process guard passed")
