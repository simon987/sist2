import unittest
import subprocess
import shutil
import json
import os

TEST_FILES = "third-party/libscan/libscan-test-files/test_files"


def copy_files(files):
    base = os.path.basename(files)
    new_path = os.path.join("/tmp/sist2_test/", base)

    shutil.rmtree(new_path, ignore_errors=True)
    shutil.copytree(files, new_path)
    return new_path


def sist2(*args):
    print("./sist2 " + " ".join(args))

    return subprocess.check_output(
        args=["./sist2", *args],
    )


def sist2_index(files, *args):
    path = copy_files(files)

    shutil.rmtree("test_i", ignore_errors=True)
    sist2("scan", path, "-o", "test_i", *args)
    return iter(sist2_index_to_dict("test_i"))


def sist2_incremental_index(files, func=None, *args):
    path = copy_files(files)

    if func:
        func(path)

    shutil.rmtree("test_i_inc", ignore_errors=True)
    sist2("scan", path, "-o", "test_i_inc", "--incremental", "test_i", *args)
    return iter(sist2_index_to_dict("test_i_inc"))


def sist2_index_to_dict(index):
    res = subprocess.check_output(
        args=["./sist2", "index", "--print", index],
    )

    for line in res.splitlines():
        if line:
            yield json.loads(line)


class ScanTest(unittest.TestCase):

    def test_incremental1(self):
        def remove_files(path):
            os.remove(os.path.join(path, "msdoc/test1.doc"))
            os.remove(os.path.join(path, "msdoc/test2.doc"))

        def add_files(path):
            with open(os.path.join(path, "newfile1"), "w"):
                pass
            with open(os.path.join(path, "newfile2"), "w"):
                pass
            with open(os.path.join(path, "newfile3"), "w"):
                pass

        file_count = sum(1 for _ in sist2_index(TEST_FILES))
        self.assertEqual(sum(1 for _ in sist2_incremental_index(TEST_FILES, remove_files)), file_count - 2)
        self.assertEqual(sum(1 for _ in sist2_incremental_index(TEST_FILES, add_files)), file_count + 3)


if __name__ == "__main__":
    unittest.main()
