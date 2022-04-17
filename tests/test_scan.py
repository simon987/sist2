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
    print("./sist2_debug " + " ".join(args))

    args = list(args)
    args.append("--fast-epub")
    # args.append("--very-verbose")

    try:
        return subprocess.check_output(
            args=["./sist2_debug", *args],
        )
    except Exception as e:
        print(e)


def sist2_index(files, *args):
    path = copy_files(files)

    shutil.rmtree("test_i", ignore_errors=True)
    sist2("scan", path, "-o", "test_i", "-t12", *args)
    return iter(sist2_index_to_dict("test_i"))


def get_lmdb_contents(path):
    import lmdb

    env = lmdb.open(path)

    txn = env.begin(write=False)

    return dict((k, v) for k, v in txn.cursor())


def sist2_incremental_index(files, func=None, incremental_index=False, *args):
    path = copy_files(files)

    if func:
        func(path)

    shutil.rmtree("test_i_inc", ignore_errors=True)
    sist2("scan", path, "-o", "test_i_inc", "--incremental", "test_i", "-t12", *args)
    return iter(sist2_index_to_dict("test_i_inc", incremental_index))


def sist2_index_to_dict(index, incremental_index=False):
    args = ["--incremental-index"] if incremental_index else []

    res = sist2("index", "--print", "--very-verbose", *args, index)

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
        lmdb_full = get_lmdb_contents("test_i/thumbs")

        # Remove files
        num_files_rm1 = len(list(sist2_incremental_index(TEST_FILES, remove_files)))
        lmdb_rm1 = get_lmdb_contents("test_i_inc/thumbs")
        self.assertEqual(num_files_rm1, file_count - 2)
        self.assertEqual(len(set(lmdb_full.keys() - set(lmdb_rm1.keys()))), 2)

        # add files (incremental_index=True)
        num_files_add_inc = len(list(sist2_incremental_index(TEST_FILES, add_files, incremental_index=True)))
        lmdb_add_inc = get_lmdb_contents("test_i_inc/thumbs")
        self.assertEqual(num_files_add_inc, 3)
        self.assertEqual(set(lmdb_full.keys()), set(lmdb_add_inc.keys()))

        # add files
        num_files_add = len(list(sist2_incremental_index(TEST_FILES, add_files)))
        lmdb_add = get_lmdb_contents("test_i_inc/thumbs")
        self.assertEqual(num_files_add, file_count + 3)
        self.assertEqual(set(lmdb_full.keys()), set(lmdb_add.keys()))

        # (No action)
        sist2_incremental_index(TEST_FILES)
        lmdb_inc = get_lmdb_contents("test_i_inc/thumbs")

        self.assertEqual(set(lmdb_full.keys()), set(lmdb_inc.keys()))


if __name__ == "__main__":
    unittest.main()
