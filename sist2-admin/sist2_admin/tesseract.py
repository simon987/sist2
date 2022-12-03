import subprocess


def get_tesseract_langs():

    res = subprocess.check_output([
        "tesseract",
        "--list-langs"
    ]).decode()

    languages = res.split("\n")[1:]

    return list(filter(lambda lang: lang and lang != "osd", languages))

