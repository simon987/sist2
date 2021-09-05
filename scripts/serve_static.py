files = [
    "sist2-vue/src/assets/favicon.ico",
    "sist2-vue/dist/css/chunk-vendors.css",
    "sist2-vue/dist/css/index.css",
    "sist2-vue/dist/js/chunk-vendors.js",
    "sist2-vue/dist/js/index.js",
    "sist2-vue/dist/index.html",
]


def clean(filepath):
    return filepath.split("/")[-1].replace(".", "_").replace("-", "_")


for file in files:
    try:
        with open(file, "rb") as f:
            data = f.read()
    except:
        data = bytes([])

    print("char %s[%d] = {%s};" % (clean(file), len(data), ",".join(str(int(b)) for b in data)))
