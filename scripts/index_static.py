import json

files = [
    "schema/mappings.json",
    "schema/settings.json",
    "schema/settings_legacy.json",
    "schema/pipeline.json",
]


def clean(filepath):
    return filepath.split("/")[-1].replace(".", "_").replace("-", "_")


for file in files:
    with open(file, "r") as f:
        data = json.dumps(json.load(f), separators=(",", ":")).encode()
    data += b'\0'
    print("char %s[%d] = {%s};" % (clean(file), len(data), ",".join(str(int(b)) for b in data)))
