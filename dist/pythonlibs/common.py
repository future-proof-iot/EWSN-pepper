import logging
import os
import sys
import shutil

import re
import logging

LOGGER = logging.getLogger()


def create_directory(directory, clean=False, mode=0o755):
    """Directory creation helper with `clean` option.

    :param clean: tries deleting the directory before re-creating it
    """
    if clean:
        try:
            shutil.rmtree(directory)
        except OSError:
            pass
    os.makedirs(directory, mode=mode, exist_ok=True)


def create_and_dump(data, directory, file_name):
    """Create file with name in 'directory' and dumps
    data do file
    """
    LOGGER.info("logging to file {}".format(file_name))
    file_path = os.path.join(directory, file_name)
    if os.path.exists(file_path):
        LOGGER.warning(f"File {file_name} already exists, overwriting")
    try:
        with open(file_path, "w") as f:
            for line in data:
                f.write("{}\n".format(line))
    except OSError as err:
        sys.exit("Failed to create a log file: {}".format(err))
    return file_path


def cleanup_termlog(filename):
    """Removes non utf-8 characters that show up when connecting to node
    over IoT-LAB"""
    with open(filename, "r+", encoding="utf-8", errors="ignore") as fp:
        lines = fp.readlines()
        m = re.search(r".*(>.*$)", lines[0])
        if m:
            fp.seek(0)
            fp.truncate()
            fp.write(m.group(1) + "\n")
            fp.writelines(lines[1:])
