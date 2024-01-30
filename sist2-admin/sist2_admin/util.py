from glob import glob
import os
from config import DATA_FOLDER


def get_old_index_files(name):
    files = glob(os.path.join(DATA_FOLDER, f"scan-{name.replace('/', '_')}-*.sist2"))
    files = list(sorted(files, key=lambda f: os.stat(f).st_mtime))
    files = files[-1:]

    return files


def tail_sync(filename, lines=1, _buffer=4098):
    with open(filename) as f:
        lines_found = []

        block_counter = -1

        while len(lines_found) < lines:
            try:
                f.seek(block_counter * _buffer, os.SEEK_END)
            except IOError:
                f.seek(0)
                lines_found = f.readlines()
                break

            lines_found = f.readlines()

            block_counter -= 1

        return lines_found[-lines:]


def pid_is_running(pid):
    try:
        os.kill(pid, 0)
    except OSError:
        return False

    return True
