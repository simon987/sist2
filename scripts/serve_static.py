files = [
    "web/css/bundle.css",
    "web/js/bundle.js",
    "web/img/bg-bars.png",
    "web/img/sprite-skin-flat.png",
    "web/search.html",
]


def clean(filepath):
    return filepath.split("/")[-1].replace(".", "_").replace("-", "_")


for file in files:
    with open(file, "rb") as f:
        data = f.read()
    print("char %s[%d] = {%s};" % (clean(file), len(data), ",".join(str(int(b)) for b in data)))
