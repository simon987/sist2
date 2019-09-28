files = [
    "schema/mappings.json",
    "schema/settings.json",
]


def clean(filepath):
    return filepath.split("/")[-1].replace(".", "_").replace("-", "_")


for file in files:
    with open(file, "rb") as f:
        data = f.read()
    print("char %s[%d] = {%s};" % (clean(file), len(data), ",".join(str(int(b)) for b in data)))
