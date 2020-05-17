files = [
    "src/static/css/bundle.css",
    "src/static/css/bundle_dark.css",
    "src/static/js/bundle.js",
    "src/static/js/search.js",
    "src/static/img/sprite-skin-flat.png",
    "src/static/img/sprite-skin-flat-dark.png",
    "src/static/search.html",
    "src/static/stats.html",
]


def clean(filepath):
    return filepath.split("/")[-1].replace(".", "_").replace("-", "_")


for file in files:
    with open(file, "rb") as f:
        data = f.read()
    print("char %s[%d] = {%s};" % (clean(file), len(data), ",".join(str(int(b)) for b in data)))
